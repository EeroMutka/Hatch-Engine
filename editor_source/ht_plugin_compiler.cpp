#include "include/ht_internal.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RestartManager.h>
#include <winternl.h>

#include <stdio.h>

static void ForceVisualStudioToClosePDBFileHandle(STR_View pdb_filepath) {
	// This function follows the method described in the article:
	// https://blog.molecular-matters.com/2017/05/09/deleting-pdb-files-locked-by-visual-studio/
	// It's... something.

	typedef NTSTATUS (NTAPI* lpNtQuerySystemInformation)(ULONG SystemInformationClass, PVOID SystemInformation, ULONG SystemInformationLength, PULONG ReturnLength);
	typedef NTSTATUS (NTAPI* lpNtDuplicateObject)(HANDLE SourceProcessHandle, HANDLE SourceHandle, HANDLE TargetProcessHandle, PHANDLE TargetHandle, ACCESS_MASK DesiredAccess, ULONG Attributes, ULONG Options);
	typedef NTSTATUS (NTAPI *lpNtQueryObject)(HANDLE Handle, OBJECT_INFORMATION_CLASS ObjectInformationClass, PVOID ObjectInformation, ULONG ObjectInformationLength, PULONG ReturnLength);

	typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO {
		USHORT UniqueProcessId;
		USHORT CreatorBackTraceIndex;
		UCHAR ObjectTypeIndex; // https://imphash.medium.com/windows-process-internals-a-few-concepts-to-know-before-jumping-on-memory-forensics-part-5-a-2368187685e
		UCHAR HandleAttributes;
		USHORT HandleValue;
		PVOID Object;
		ULONG GrantedAccess;
	} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

	typedef struct _SYSTEM_HANDLE_INFORMATION
	{
		ULONG NumberOfHandles;
		SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
	} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

	struct OBJECT_NAME_INFORMATION {
		UNICODE_STRING Name; // defined in winternl.h
		WCHAR NameBuffer;
	};

	DWORD dwSession;
	WCHAR szSessionKey[CCH_RM_SESSION_KEY+1] = { 0 };
	DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);

	STR_View pdb_filename = STR_AfterLast(pdb_filepath, '/');
	const wchar_t* pdb_filename_w = OS_UTF8ToWide(MEM_SCOPE(TEMP), pdb_filename, 1);
	const wchar_t* pdb_filepath_w = OS_UTF8ToWide(MEM_SCOPE(TEMP), pdb_filepath, 1);

	if (dwError == ERROR_SUCCESS) {
		dwError = RmRegisterResources(dwSession, 1, &pdb_filepath_w, 0, NULL, 0, NULL);

		if (dwError == ERROR_SUCCESS) {
			DWORD dwReason;
			UINT nProcInfoNeeded;
			UINT nProcInfo = 10;
			RM_PROCESS_INFO rgpi[10];
			dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, rgpi, &dwReason);
			//wprintf(L"RmGetList returned %d\n", dwError);

			if (dwError == ERROR_SUCCESS) {
				//wprintf(L"RmGetList returned %d infos (%d needed)\n", nProcInfo, nProcInfoNeeded);

				for (UINT i = 0; i < nProcInfo; i++) {
					//wprintf(L"%d.ApplicationType = %d\n", i, rgpi[i].ApplicationType);
					//wprintf(L"%d.strAppName = %ls\n", i, rgpi[i].strAppName);
					//wprintf(L"%d.Process.dwProcessId = %d\n", i, rgpi[i].Process.dwProcessId);
					DWORD pid = rgpi[i].Process.dwProcessId;
					USHORT pid_u16 = (USHORT)pid;
					assert(pid_u16 == pid);

					HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
					if (hProcess) {
						const size_t SystemHandleInformationSize = 4096 * 4096 * 2;

						// https://www.ired.team/miscellaneous-reversing-forensics/windows-kernel-internals/get-all-open-handles-and-kernel-object-address-from-userland
						ULONG returnLenght = 0;
						HMODULE ntdll = GetModuleHandleW(L"ntdll");
						lpNtQuerySystemInformation QuerySystemInformation = (lpNtQuerySystemInformation)GetProcAddress(ntdll, "NtQuerySystemInformation");
						lpNtDuplicateObject DuplicateObject = (lpNtDuplicateObject)GetProcAddress(ntdll, "NtDuplicateObject");
						lpNtQueryObject QueryObject = (lpNtQueryObject)GetProcAddress(ntdll, "NtQueryObject");

						PSYSTEM_HANDLE_INFORMATION handleTableInformation = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, SystemHandleInformationSize);
						
						QuerySystemInformation((SYSTEM_INFORMATION_CLASS)0x10 /*SystemHandleInformation*/, handleTableInformation, SystemHandleInformationSize, &returnLenght);


						for (ULONG j = 0; j < handleTableInformation->NumberOfHandles; j++) {
							SYSTEM_HANDLE_TABLE_ENTRY_INFO* handleInfo = &handleTableInformation->Handles[j];

							if (handleInfo->UniqueProcessId == pid_u16 /*&& handleInfo->ObjectTypeIndex == 42 */&&
								// Through experimentation, I noticed that there's always a NamedPipe handle object as the last element that blocks the QueryObject call. We can filter it out by looking at the GrantedAccess. I'm not entirely sure what the access flags mean, but the correct access. I got was 0x120089 and the blocking access to filter out was 0x120189. So this is just a very dumb filter that might break in the future.
								handleInfo->GrantedAccess == 0x120089)
							{
								//printf_s("UniqueProcessId %x  CreatorBackTraceIndex %x  ObjectTypeIndex %x  HandleAttributes %x  HandleValue %x  Object %p  GrantedAccess %x\n", handleInfo->UniqueProcessId, handleInfo->CreatorBackTraceIndex, handleInfo->ObjectTypeIndex, handleInfo->HandleAttributes, handleInfo->HandleValue, handleInfo->Object, handleInfo->GrantedAccess);

								HANDLE dupHandle = NULL;
								HANDLE NtCurrentProcess = ((HANDLE)(LONG_PTR)-1);
								NTSTATUS status = DuplicateObject(hProcess, (HANDLE)handleInfo->HandleValue, NtCurrentProcess, &dupHandle, STANDARD_RIGHTS_READ|SYNCHRONIZE, 0, 0);
								
								if (!NT_SUCCESS(status)) continue;

								BYTE  u8_Buffer[1024];
								DWORD u32_ReqLength = 0;

								UNICODE_STRING* pk_Info = &((OBJECT_NAME_INFORMATION*)u8_Buffer)->Name;
								pk_Info->Buffer = 0;
								pk_Info->Length = 0;

								// Note by a StackOverflow user: The return value from NtQueryObject is bullshit! (driver bug?)
								// - The function may return STATUS_NOT_SUPPORTED although it has successfully written to the buffer.
								// - The function returns STATUS_SUCCESS although h_File == 0xFFFFFFFF
								QueryObject(dupHandle, (OBJECT_INFORMATION_CLASS)1 /*ObjectNameInformation*/, u8_Buffer, sizeof(u8_Buffer), &u32_ReqLength);
								
								if (!pk_Info->Buffer || !pk_Info->Length) {} // fail...
								else {
									int str_len = pk_Info->Length / 2;
									pk_Info->Buffer[str_len] = 0;

									if (str_len >= pdb_filename.size &&
										wcscmp(pk_Info->Buffer + str_len - pdb_filename.size, pdb_filename_w) == 0)
									{
										// Found it! Close the handle now.
										BOOL ok = DuplicateHandle(hProcess, (HANDLE)handleInfo->HandleValue, NULL, NULL, 0, 0, DUPLICATE_CLOSE_SOURCE);
										assert(ok);
										//wprintf(L"OBJECT: %s\n", pk_Info->Buffer);
									}
								}

								CloseHandle(dupHandle);
							}
						}

						HeapFree(GetProcessHeap(), 0, handleTableInformation);
						CloseHandle(hProcess);
					}
				}
			}
		}
		RmEndSession(dwSession);
	}

	HANDLE h = CreateFileW(pdb_filepath_w, 0, 0, NULL, OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0);
	assert(h != INVALID_HANDLE_VALUE);
	CloseHandle(h);
}

EXPORT void RecompilePlugin(EditorState* s, Asset* plugin, STR_View hatch_install_directory) {
	Asset* package = plugin;
	for (;package->kind != AssetKind_Package; package = package->parent) {}

	if (package != plugin->parent) TODO();

	bool ok = OS_SetWorkingDir(MEM_TEMP(), package->package.filesys_path);
	assert(ok);

	STR_View plugin_name = UI_TextToStr(plugin->name);

	if (plugin->plugin.dll_handle) {
		printf("Unloading plugin with %d allocations\n", plugin->plugin.allocations.count);
		UnloadPlugin(s, plugin);

		OS_UnloadDLL(plugin->plugin.dll_handle);
		
		plugin->plugin.dll_handle = NULL;
		plugin->plugin.LoadPlugin = NULL;
		plugin->plugin.UnloadPlugin = NULL;
		plugin->plugin.UpdatePlugin = NULL;

		for (int i = 0; i < plugin->plugin.allocations.count; i++) {
			PluginAllocationHeader* allocation = plugin->plugin.allocations[i];
			DS_MemFree(DS_HEAP, allocation);
		}
		DS_ArrClear(&plugin->plugin.allocations);

		STR_View pdb_filepath = STR_Form(TEMP, ".plugin_binaries/%v.pdb", plugin_name);
		ForceVisualStudioToClosePDBFileHandle(pdb_filepath);
	}

	const char* header_name = STR_FormC(TEMP, "%v.inc.ht", plugin_name);
	FILE* header = fopen(header_name, "wb");
	assert(header != NULL);

	fprintf(header, "// This file is generated by Hatch. Do not edit by hand.\n");
	fprintf(header, "#pragma once\n\n");
	fprintf(header, "#include <hatch_api.h>\n\n");

	PluginOptions* plugin_opts = &plugin->plugin.options;

	Asset* plugin_data = GetAsset(&s->asset_tree, plugin_opts->data);
	if (plugin_data) {
		assert(AssetIsValid(&s->asset_tree, plugin_data->struct_data.struct_type));
		//Asset* plugin_data_type = plugin_data->struct_data.struct_type.asset;
	}

	// TODO: so we need a recursive list of all types used inside this type...

	// for now, do the simple flat alternative that doesn't work in many cases.
	for (Asset* child = plugin->parent->first_child; child; child = child->next) {
		if (child->kind == AssetKind_StructType) {
			STR_View name = UI_TextToStr(child->name);
			fprintf(header, "typedef struct %.*s {\n", StrArg(name));

			for (int i = 0; i < child->struct_type.members.count; i++) {
				StructMember member = child->struct_type.members[i];
				fprintf(header, "\t");
				switch (member.type.kind) {
				case TypeKind_Float: { fprintf(header, "float"); }break;
				case TypeKind_Int: { fprintf(header, "int"); }break;
				case TypeKind_Bool: { fprintf(header, "bool"); }break;
				case TypeKind_AssetRef: { fprintf(header, "HT_AssetHandle"); }break;
				default: assert(0); break;
				}
				STR_View member_name = UI_TextToStr(member.name.text);
				fprintf(header, " %.*s;\n", StrArg(member_name));
			}

			fprintf(header, "} %.*s;\n", StrArg(name));
		}
	}

	fclose(header);

	BUILD_ProjectOptions opts = {};
	opts.target = BUILD_Target_DynamicLibrary;
	
	// When running inside visual studio's debugger, VS keeps the DLL pdb file open even after unloading the DLL.
	// For now, just disable debug info.
	// https://blog.molecular-matters.com/2017/05/09/deleting-pdb-files-locked-by-visual-studio/
	opts.debug_info = true;

	// TODO: it would be nice to make this folder at the root of the package. That way, you could more easily delete the binaries or gitignore them.
	ok = BUILD_CreateDirectory(".plugin_binaries");
	assert(ok);

	BUILD_Project project;
	BUILD_ProjectInit(&project, STR_ToC(TEMP, plugin_name), &opts);

	BUILD_AddIncludeDir(&project, STR_FormC(TEMP, "%v/plugin_include", hatch_install_directory)); // for hatch_types.h

	for (int i = 0; i < plugin_opts->source_files.count; i++) {
		HT_AssetHandle source_file = *((HT_AssetHandle*)plugin_opts->source_files.data + i);
		Asset* source_file_asset = GetAsset(&s->asset_tree, source_file);
		if (source_file_asset) {
			const char* file_name = STR_ToC(TEMP, UI_TextToStr(source_file_asset->name));
			BUILD_AddSourceFile(&project, file_name);
		}
	}
	if (project.source_files.count == 0) TODO();

	//PluginBuildLog build_log = {0};
	//build_log.plugin = new_plugin_data;
	//build_log.str_builder.arena = UI_FrameArena();
	//build_log.base.print = PluginBuildLogPrint;
	printf("\n\n");
	ok = BUILD_CompileProject(&project, ".plugin_binaries", ".", BUILD_GetConsole());

	if (ok) {
		//wchar_t cwd[64];
		//GetCurrentDirectoryW(64, cwd);

		STR_View dll_path = STR_Form(TEMP, ".plugin_binaries\\%v.dll", plugin_name);
		OS_DLL* dll = OS_LoadDLL(MEM_TEMP(), dll_path);
		ok = dll != NULL;
		assert(ok);

		plugin->plugin.dll_handle = dll;

		*(void**)&plugin->plugin.LoadPlugin = OS_GetProcAddress(dll, "HT_LoadPlugin");
		*(void**)&plugin->plugin.UnloadPlugin = OS_GetProcAddress(dll, "HT_UnloadPlugin");
		*(void**)&plugin->plugin.UpdatePlugin = OS_GetProcAddress(dll, "HT_UpdatePlugin");

		LoadPlugin(s, plugin);
	}

	BUILD_ProjectDeinit(&project);
}

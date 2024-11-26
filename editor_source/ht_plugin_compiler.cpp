#define _CRT_SECURE_NO_WARNINGS // for fopen

#include "include/ht_common.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <RestartManager.h>
#include <winternl.h>

#include <stdio.h>

EXPORT void ForceVisualStudioToClosePDBFileHandle(STR_View pdb_filepath) {
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
	const wchar_t* pdb_filename_w = OS_UTF8ToWide(MEM_SCOPE_TEMP, pdb_filename, 1);
	const wchar_t* pdb_filepath_w = OS_UTF8ToWide(MEM_SCOPE_TEMP, pdb_filepath, 1);

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
					ASSERT(pid_u16 == pid);

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
										ASSERT(ok);
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
	ASSERT(h != INVALID_HANDLE_VALUE);
	CloseHandle(h);
}

struct BuildLog {
	BUILD_Log base;
	ErrorList* error_list;
	HT_Asset plugin;
	STR_Builder b;
	size_t flushed_to;
};

static void FlushBuildLog(BuildLog* build_log) {
	for (;;) {
		STR_View remaining = STR_SliceAfter(build_log->b.str, build_log->flushed_to);

		size_t at;
		if (!STR_Find(remaining, "\n", &at)) break;

		STR_View line = STR_SliceBefore(remaining, at);
		build_log->flushed_to += at + 1;

		size_t _;
		bool is_error = STR_Find(line, ": error ", &_);
		
		if (is_error) {
			Error error = {};
			error.owner_asset = build_log->plugin;
			error.string = STR_Clone(DS_HEAP, line);
			error.added_tick = OS_GetCPUTick();
			DS_ArrPush(&build_log->error_list->errors, error);
		}

		//Log* log = build_log->log;
		//LogMessage msg;
		//msg.kind = is_error ? LogMessageKind_Error : LogMessageKind_Info;
		//msg.string = STR_Clone(&log->arena, line);
		//msg.added_tick = OS_GetCPUTick();
		//DS_ArrPush(&log->messages, msg);
	}
}

static void BuildLogFn(BUILD_Log* self, const char* message) {
	BuildLog* build_log = (BuildLog*)self;
	STR_Print(&build_log->b, STR_ToV(message));
	FlushBuildLog(build_log);
}

EXPORT bool RecompilePlugin(EditorState* s, Asset* plugin) {
	ASSERT(plugin->plugin.active_instance == NULL); // plugin must not be running
	
	Asset* package = plugin;
	for (;package->kind != AssetKind_Package; package = package->parent) {}
	bool ok = OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);
	ASSERT(ok);

	STR_View plugin_name = UI_TextToStr(plugin->name);

	/*if (plugin->plugin.dll_handle) {
		printf("Unloading plugin with %d allocations\n", plugin->plugin.allocations.count);
		UnloadPlugin(s, plugin);
	}*/

	const char* header_name = STR_FormC(TEMP, "%v.inc.ht", plugin_name);
	FILE* header = fopen(header_name, "wb");
	ASSERT(header != NULL);

	fprintf(header, "// This file is generated by Hatch. Do not edit by hand.\n");
	fprintf(header, "#pragma once\n\n");
	//fprintf(header, "#include <hatch_api.h>\n\n");

	PluginOptions* plugin_opts = &plugin->plugin.options;

	Asset* plugin_data = GetAsset(&s->asset_tree, plugin_opts->data);
	if (plugin_data) {
		ASSERT(plugin_data->kind == AssetKind_StructData);
		ASSERT(GetAsset(&s->asset_tree, plugin_data->struct_data.struct_type) != NULL);
		//Asset* plugin_data_type = plugin_data->struct_data.struct_type.asset;
	}

	// for now, do the simple way that doesn't work in many cases.
	// This is totally wrong.
	for (DS_BkArrEach(&s->asset_tree.assets, asset_i)) {
		Asset* asset = DS_BkArrGet(&s->asset_tree.assets, asset_i);

		if (asset->kind == AssetKind_StructType) {
			STR_View name = UI_TextToStr(asset->name);
			if (STR_Match(name, "Untitled Struct")) continue; // temporary hack against builtin structures

			fprintf(header, "typedef struct %.*s {\n", StrArg(name));

			for (int i = 0; i < asset->struct_type.members.count; i++) {
				StructMember member = asset->struct_type.members[i];
				fprintf(header, "\t");
				switch (member.type.kind) {
				case HT_TypeKind_Float: { fprintf(header, "float"); }break;
				case HT_TypeKind_Int: { fprintf(header, "int"); }break;
				case HT_TypeKind_Bool: { fprintf(header, "bool"); }break;
				case HT_TypeKind_String: { fprintf(header, "string"); }break;
				case HT_TypeKind_Type: { fprintf(header, "HT_Type"); }break;
				case HT_TypeKind_Array: { fprintf(header, "HT_Array"); }break;
				case HT_TypeKind_ItemGroup: { fprintf(header, "HT_ItemGroup"); }break;
				case HT_TypeKind_Struct: {
					fprintf(header, "long long");
				}break;
				case HT_TypeKind_AssetRef: { fprintf(header, "HT_Asset"); }break;
				case HT_TypeKind_Vec2: { fprintf(header, "vec2"); }break;
				case HT_TypeKind_Vec3: { fprintf(header, "vec3"); }break;
				case HT_TypeKind_Vec4: { fprintf(header, "vec4"); }break;
				case HT_TypeKind_IVec2: { fprintf(header, "ivec2"); }break;
				case HT_TypeKind_IVec3: { fprintf(header, "ivec3"); }break;
				case HT_TypeKind_IVec4: { fprintf(header, "ivec4"); }break;
				default: ASSERT(0); break;
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
	ASSERT(ok);

	BUILD_Project project;
	BUILD_ProjectInit(&project, STR_ToC(TEMP, plugin_name), &opts);

	BUILD_AddIncludeDir(&project, STR_FormC(TEMP, "%v/plugin_include", s->hatch_install_directory));

	for (int i = 0; i < plugin_opts->code_files.count; i++) {
		HT_Asset code_file = *((HT_Asset*)plugin_opts->code_files.data + i);
		Asset* code_file_asset = GetAsset(&s->asset_tree, code_file);
		if (code_file_asset) {
			STR_View file_name = AssetGetPackageRelativePath(TEMP, code_file_asset);
			STR_View file_extension = STR_AfterLast(file_name, '.');
			
			// Only add .c and .cpp files as translation units and not header files for example
			if (STR_Match(file_extension, "c") || STR_Match(file_extension, "cpp")) {
				const char* file_name_c = STR_ToC(TEMP, file_name);
				BUILD_AddSourceFile(&project, file_name_c);
			}
		}
	}
	if (project.code_files.count == 0) TODO();

	RemoveErrorsByAsset(&s->error_list, plugin->handle);

	BuildLog build_log = {0};
	build_log.base.print = BuildLogFn;
	build_log.plugin = plugin->handle;
	build_log.error_list = &s->error_list;
	build_log.b = {TEMP};
	ok = BUILD_CompileProject(&project, ".plugin_binaries", ".", &build_log.base);
	
	FlushBuildLog(&build_log);

	BUILD_ProjectDeinit(&project);
	return ok;
}

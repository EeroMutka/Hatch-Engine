#include <ht_utils/fire/fire_ds.h>
#include <ht_utils/fire/fire_string.h>

#include "os_misc.h"

#include <shobjidl_core.h> // required for OS_FolderPicker

#include <stdio.h> // for fopen

OS_API void OS_ReadEntireFile(DS_MemScope* m, const char* file, STR_View* out_data) {
	FILE* f = NULL;
	errno_t err = fopen_s(&f, file, "rb");
	assert(f); // TODO

	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);

	char* data = DS_ArenaPush(m->arena, fsize);
	fread(data, fsize, 1, f);

	fclose(f);
	STR_View result = {data, fsize};
	*out_data = result;
}

OS_API bool OS_PathIsAbsolute(STR_View path) {
	return path.size > 2 && path.data[1] == ':';
}

OS_API wchar_t* OS_UTF8ToWide(DS_MemScope* m, STR_View str, int null_terminations) {
	if (str.size == 0) return L""; // MultiByteToWideChar does not accept 0-length strings

	int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data, (int)str.size, NULL, 0);
	wchar_t* result = (wchar_t*)DS_ArenaPush(m->arena, (size + null_terminations) * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str.data, (int)str.size, result, size);

	for (int i = 0; i < null_terminations; i++) {
		result[size + i] = 0;
	}
	return result;
}

OS_API void OS_WideToUTF8(DS_MemScope* m, const wchar_t* wstr, STR_View* out_string) {
	if (*wstr == 0) {
		*out_string = STR_V(""); // MultiByteToWideChar does not accept 0-length strings
		return;
	}

	int buffer_size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* new_data = DS_ArenaPush(m->arena, buffer_size);
	WideCharToMultiByte(CP_UTF8, 0, wstr, -1, new_data, buffer_size, NULL, NULL);
	
	STR_View result = {new_data, buffer_size - 1};
	*out_string = result;
}

OS_API bool OS_DeleteFile(DS_MemTemp* m, STR_View file_path) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	bool ok = DeleteFileW(OS_UTF8ToWide(&temp, file_path, 1)) == 1;
	DS_ScopeEnd(&temp);
	return ok;
}

OS_API bool OS_FileGetModtime(DS_MemTemp* m, STR_View file_path, uint64_t* out_modtime) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	wchar_t* file_path_wide = OS_UTF8ToWide(&temp, file_path, 1);

	// TODO: use GetFileAttributesExW like https://github.com/mmozeiko/TwitchNotify/blob/master/TwitchNotify.c#L568-L583
	HANDLE h = CreateFileW(file_path_wide, 0, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		GetFileTime(h, NULL, NULL, (FILETIME*)out_modtime);
		CloseHandle(h);
	}

	DS_ScopeEnd(&temp);
	return h != INVALID_HANDLE_VALUE;
}

OS_API void OS_DeleteDirectory(DS_MemTemp* m, STR_View directory_path) {
	DS_MemScope temp = DS_ScopeBeginT(m);

	SHFILEOPSTRUCTW file_op = {0};
	file_op.hwnd = NULL;
	file_op.wFunc = FO_DELETE;
	file_op.pFrom = OS_UTF8ToWide(&temp, directory_path, 2); // NOTE: pFrom must be double null-terminated!
	file_op.pTo = NULL;
	file_op.fFlags = FOF_NO_UI;
	file_op.fAnyOperationsAborted = false;
	file_op.hNameMappings = 0;
	file_op.lpszProgressTitle = NULL;
	int res = SHFileOperationW(&file_op);

	DS_ScopeEnd(&temp);
}

OS_API bool OS_PathToCanonical(DS_MemScope* m, STR_View path, STR_View* out_path) {
	// https://pdh11.blogspot.com/2009/05/pathcanonicalize-versus-what-it-says-on.html
	// https://stackoverflow.com/questions/10198420/open-directory-using-createfile

	DS_MemScope temp = DS_ScopeBegin(m);
	wchar_t* path_wide = OS_UTF8ToWide(&temp, path, 1);

	HANDLE file_handle = CreateFileW(path_wide, 0, 0, NULL, OPEN_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	bool dummy_file_was_created = GetLastError() == 0;
	bool ok = file_handle != INVALID_HANDLE_VALUE;

	wchar_t result_wide[MAX_PATH];
	if (ok) ok = GetFinalPathNameByHandleW(file_handle, result_wide, MAX_PATH, VOLUME_NAME_DOS) < MAX_PATH;

	if (file_handle != INVALID_HANDLE_VALUE) {
		CloseHandle(file_handle);
	}

	if (dummy_file_was_created) {
		DeleteFileW(result_wide);
	}

	if (ok) {
		wchar_t* result_wide_cut = result_wide + 4; // strings returned have `\\?\` - prefix that we want to get rid of
		OS_WideToUTF8(m, result_wide_cut, out_path);
	}

	DS_ScopeEnd(&temp);
	return ok;
}

OS_API bool OS_MakeDirectory(DS_MemTemp* m, STR_View directory) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	wchar_t* dir_wide = OS_UTF8ToWide(&temp, directory, 1);
	bool created = CreateDirectoryW(dir_wide, NULL);
	DS_ScopeEnd(&temp);
	return created || GetLastError() == ERROR_ALREADY_EXISTS;
}

OS_API bool OS_SetWorkingDir(DS_MemTemp* m, STR_View directory) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	wchar_t* dir_wide = OS_UTF8ToWide(&temp, directory, 1);
	bool ok = SetCurrentDirectoryW(dir_wide) != 0;
	DS_ScopeEnd(&temp);
	return ok;
}

OS_API bool OS_FileLastModificationTime(DS_MemTemp* m, STR_View filepath, uint64_t* out_modtime) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	wchar_t* filepath_wide = OS_UTF8ToWide(&temp, filepath, 1);

	HANDLE h = CreateFileW(filepath_wide, 0, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_BACKUP_SEMANTICS, NULL);
	bool ok = h != INVALID_HANDLE_VALUE;
	if (ok) {
		ok = GetFileTime(h, NULL, NULL, (FILETIME*)out_modtime) != 0;
		CloseHandle(h);
	}

	DS_ScopeEnd(&temp);
	return ok;
}

#ifdef __cplusplus
#define OS_COM_PTR_CALL(OBJECT, FN, ...) OBJECT->FN(__VA_ARGS__)
#else
#define OS_COM_PTR_CALL(OBJECT, FN, ...) OBJECT->lpVtbl->FN(OBJECT, __VA_ARGS__)
#endif

OS_API bool OS_FolderPicker(DS_MemScope* m, STR_View* out_path) {
	bool ok = false;
	if (SUCCEEDED(CoInitialize(NULL))) {
		IFileDialog* dialog;

#ifdef __cplusplus
		HRESULT instance_result = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void**)&dialog);
#else
		HRESULT instance_result = CoCreateInstance(&CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, &IID_IFileOpenDialog, &dialog);
#endif

		if (SUCCEEDED(instance_result)) {
			// NOTE: for the following to compile, the CINTERFACE macro must be defined before the Windows COM headers are included!

			OS_COM_PTR_CALL(dialog, SetOptions, FOS_PICKFOLDERS);
			OS_COM_PTR_CALL(dialog, Show, NULL);

			IShellItem* dialog_result;
			if (SUCCEEDED(OS_COM_PTR_CALL(dialog, GetResult, &dialog_result))) {
				wchar_t* path_wide = NULL;
				if (SUCCEEDED(OS_COM_PTR_CALL(dialog_result, GetDisplayName, SIGDN_FILESYSPATH, &path_wide))) {
					ok = true;

					OS_WideToUTF8(m, path_wide, out_path);

					char* data = (char*)out_path->data;
					for (int i = 0; i < out_path->size; i++) {
						if (data[i] == '\\') data[i] = '/';
					}

					CoTaskMemFree(path_wide);
				}
				OS_COM_PTR_CALL(dialog_result, Release);
			}
			OS_COM_PTR_CALL(dialog, Release);
		}
		CoUninitialize();
	}
	return ok;
}

OS_API bool OS_GetAllFilesInDirectory(DS_MemScope* m, STR_View directory, OS_FileInfoArray* out_files) {
	DS_MemScope temp = DS_ScopeBegin(m);

	char* match_str_data = DS_ArenaPush(temp.arena, directory.size + 2);
	memcpy(match_str_data, directory.data, directory.size);
	match_str_data[directory.size] = '\\';
	match_str_data[directory.size + 1] = '*';
	STR_View match_str = {match_str_data, directory.size + 2};
	wchar_t* match_wstr = OS_UTF8ToWide(&temp, match_str, 1);

	DS_DynArray(OS_FileInfo) file_infos = { m->arena };

	WIN32_FIND_DATAW find_info;
	HANDLE handle = FindFirstFileW(match_wstr, &find_info);

	bool ok = handle != INVALID_HANDLE_VALUE;
	if (ok) {
		for (; FindNextFileW(handle, &find_info);) {
			OS_FileInfo info = {0};
			OS_WideToUTF8(m, find_info.cFileName, &info.name);
			info.is_directory = find_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
			info.last_write_time = *(uint64_t*)&find_info.ftLastWriteTime;
			if (info.name.size == 2 && info.name.data[0] == '.' && info.name.data[1] == '.') continue;

			DS_ArrPush(&file_infos, info);
		}
		ok = GetLastError() == ERROR_NO_MORE_FILES;
		FindClose(handle);
	}

	DS_ScopeEnd(&temp);

	out_files->data = file_infos.data;
	out_files->count = file_infos.count;
	return ok;
}

OS_API bool OS_FilePicker(DS_MemScope* m, STR_View* out_path) {
	wchar_t buffer[MAX_PATH];
	buffer[0] = 0;

	OPENFILENAMEW ofn = {0};
	ofn.lStructSize = sizeof(OPENFILENAMEW);
	ofn.hwndOwner = NULL;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = MAX_PATH - 1;
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	GetOpenFileNameW(&ofn);
	
	OS_WideToUTF8(m, buffer, out_path);
	return out_path->size > 0;
}

OS_API void OS_GetThisExecutablePath(DS_MemScope* m, STR_View* out_path) {
	wchar_t buf[MAX_PATH];
	uint32_t n = GetModuleFileNameW(NULL, buf, MAX_PATH);
	assert(n > 0 && n < MAX_PATH);
	OS_WideToUTF8(m, buf, out_path);
}

OS_API void OS_UnloadDLL(OS_DLL* dll) {
	bool ok = FreeLibrary((HINSTANCE)dll);
	assert(ok);
}

OS_API OS_DLL* OS_LoadDLL(DS_MemTemp* m, STR_View dll_path) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	
	wchar_t* dll_path_wide = OS_UTF8ToWide(&temp, dll_path, 1);
	HANDLE handle = LoadLibraryW(dll_path_wide);

	DS_ScopeEnd(&temp);
	return (OS_DLL*)handle;
}

OS_API void* OS_GetProcAddress(OS_DLL* dll, const char* name) {
	return GetProcAddress((HMODULE)dll, name);
}


#ifndef OS_API
#define OS_API
#endif

OS_API void OS_ReadEntireFile(DS_MemScope* m, const char* file, STR_View* out_data);

OS_API bool OS_PathIsAbsolute(STR_View path);

OS_API wchar_t* OS_UTF8ToWide(DS_MemScope* m, STR_View str, int null_terminations);

OS_API void OS_WideToUTF8(DS_MemScope* m, const wchar_t* wstr, STR_View* out_string);

OS_API bool OS_DeleteFile(DS_MemScopeNone* m, STR_View file_path);

OS_API bool OS_FileGetModtime(DS_MemScopeNone* m, STR_View file_path, uint64_t* out_modtime);

OS_API bool OS_RunProcess(DS_MemScopeNone* m, STR_View command_string, uint32_t* out_exit_code);

OS_API void OS_DeleteDirectory(DS_MemScopeNone* m, STR_View directory_path);

OS_API bool OS_PathToCanonical(DS_MemScope* m, STR_View path, STR_View* out_path);

OS_API bool OS_MakeDirectory(DS_MemScopeNone* m, STR_View directory);

OS_API bool OS_SetWorkingDir(DS_MemScopeNone* m, STR_View directory);

OS_API bool OS_FileLastModificationTime(DS_MemScopeNone* m, STR_View filepath, uint64_t* out_modtime);

OS_API bool OS_FilePicker(DS_MemScope* m, STR_View* out_path);

OS_API bool OS_FolderPicker(DS_MemScope* m, STR_View* out_path);

typedef struct OS_FileInfo {
	bool is_directory;
	STR_View name; // includes the file extension if there is one
	uint64_t last_write_time;
} OS_FileInfo;

typedef struct { OS_FileInfo* data; int count; } OS_FileInfoArray;

OS_API bool OS_GetAllFilesInDirectory(DS_MemScope* m, STR_View directory, OS_FileInfoArray* out_files);

OS_API void OS_GetThisExecutablePath(DS_MemScope* m, STR_View* out_path);

typedef struct OS_DLL OS_DLL;

OS_API OS_DLL* OS_LoadDLL(DS_MemScopeNone* m, STR_View dll_path); // may return NULL

OS_API void OS_UnloadDLL(OS_DLL* dll);

OS_API void* OS_GetProcAddress(OS_DLL* dll, const char* name);

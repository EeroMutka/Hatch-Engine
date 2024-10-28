
EXPORT STR_View OS_ReadEntireFile(DS_MemScope* m, const char* file);

EXPORT bool OS_PathIsAbsolute(STR_View path);

EXPORT wchar_t* OS_UTF8ToWide(DS_MemScope* m, STR_View str, int null_terminations);

EXPORT STR_View OS_WideToUTF8(DS_MemScope* m, const wchar_t* wstr);

EXPORT bool OS_DeleteFile(DS_MemTemp* m, STR_View file_path);

EXPORT bool OS_FileGetModtime(DS_MemTemp* m, STR_View file_path, uint64_t* out_modtime);

EXPORT void OS_DeleteDirectory(DS_MemTemp* m, STR_View directory_path);

EXPORT bool OS_PathToCanonical(DS_MemScope* m, STR_View path, STR_View* out_path);

EXPORT bool OS_MakeDirectory(DS_MemTemp* m, STR_View directory);

EXPORT bool OS_SetWorkingDir(DS_MemTemp* m, STR_View directory);

EXPORT bool OS_FileLastModificationTime(DS_MemTemp* m, STR_View filepath, uint64_t* out_modtime);

EXPORT bool OS_FilePicker(DS_MemTemp* m, STR_View* out_path);

EXPORT bool OS_FolderPicker(DS_MemScope* m, STR_View* out_path);

typedef struct OS_FileInfo {
	bool is_directory;
	STR_View name; // includes the file extension if there is one
	uint64_t last_write_time;
} OS_FileInfo;

typedef struct { OS_FileInfo* data; int count; } OS_FileInfoArray;

EXPORT bool OS_GetAllFilesInDirectory(DS_MemScope* m, STR_View directory, OS_FileInfoArray* out_files);

EXPORT STR_View OS_GetThisExecutablePath(DS_MemScope* m);

typedef struct OS_DLL OS_DLL;

EXPORT OS_DLL* OS_LoadDLL(DS_MemTemp* m, STR_View dll_path); // may return NULL

EXPORT void OS_UnloadDLL(OS_DLL* dll);

EXPORT void* OS_GetProcAddress(OS_DLL* dll, const char* name);

#ifndef OS_API
#define OS_API
#endif

typedef struct { void* handle; } OS_DirectoryWatch;

OS_API bool OS_InitDirectoryWatch(DS_Info* ds, OS_DirectoryWatch* watch, STR_View directory);

OS_API void OS_DeinitDirectoryWatch(OS_DirectoryWatch* watch); // you may call this on a zero/deinitialized OS_DirectoryWatch

OS_API bool OS_DirectoryWatchHasChanges(OS_DirectoryWatch* watch);

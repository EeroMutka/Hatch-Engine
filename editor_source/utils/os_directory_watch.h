
typedef struct { void* handle; } OS_DirectoryWatch;

static bool OS_InitDirectoryWatch(DS_MemTemp* m, OS_DirectoryWatch* watch, STR_View directory);

static void OS_DeinitDirectoryWatch(OS_DirectoryWatch* watch); // you may call this on a zero/deinitialized OS_DirectoryWatch

static bool OS_DirectoryWatchHasChanges(OS_DirectoryWatch* watch);

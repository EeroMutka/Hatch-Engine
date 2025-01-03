#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>
#include <ht_utils/fire/fire_string.h>

#include "os_misc.h"
#include "os_directory_watch.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

OS_API bool OS_InitDirectoryWatch(DS_Info* ds, OS_DirectoryWatch* watch, STR_View directory) {
	DS_ArenaMark mark = DS_ArenaGetMark(ds->temp_arena);
	
	wchar_t* directory_wide = OS_UTF8ToWide(ds->temp_arena, directory, 1);

	HANDLE handle = FindFirstChangeNotificationW(directory_wide, TRUE,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY);
	watch->handle = handle;

	DS_ArenaSetMark(ds->temp_arena, mark);
	return handle != INVALID_HANDLE_VALUE;
}

OS_API void OS_DeinitDirectoryWatch(OS_DirectoryWatch* watch) {
	if (watch->handle) {
		bool ok = FindCloseChangeNotification(watch->handle);
		assert(ok);
		watch->handle = 0;
	}
}

OS_API bool OS_DirectoryWatchHasChanges(OS_DirectoryWatch* watch) {
	DWORD wait = WaitForSingleObject((HANDLE)watch->handle, 0);
	if (wait == WAIT_OBJECT_0) {
		bool ok = FindNextChangeNotification((HANDLE)watch->handle);
		assert(ok);
	}
	return wait == WAIT_OBJECT_0;
}

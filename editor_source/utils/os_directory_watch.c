#include "os_directory_watch.h"

OS_API bool OS_InitDirectoryWatch(DS_MemTemp* m, OS_DirectoryWatch* watch, STR_View directory) {
	DS_MemScope temp = DS_ScopeBeginT(m);
	wchar_t* directory_wide = OS_UTF8ToWide(&temp, directory, 1);

	HANDLE handle = FindFirstChangeNotificationW(directory_wide, TRUE,
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SECURITY);
	watch->handle = handle;

	DS_ScopeEnd(&temp);
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

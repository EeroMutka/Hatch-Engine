#define EXPORT

#include "fire/fire_ds.h"
#include "fire/fire_string.h"

#define BUILD_API
#define FIRE_BUILD_IMPLEMENTATION
#include "fire/fire_build.h"

#define FIRE_OS_WINDOW_IMPLEMENTATION
#include "fire/fire_os_window.h"

#define FIRE_OS_CLIPBOARD_IMPLEMENTATION
#include "fire/fire_os_clipboard.h"

#include "utils/os_misc.c"
#include "utils/os_directory_watch.c"

#include "fire/fire_ui/fire_ui.h"
#include "fire/fire_ui/fire_ui.c"
#include "fire/fire_ui/fire_ui_color_pickers.c"
#include "fire/fire_ui/fire_ui_extras.c"
#include "utils/ui_data_tree.c"
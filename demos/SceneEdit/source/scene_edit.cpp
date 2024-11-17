#include <hatch_api.h>
// #include "../scene_edit.inc.ht"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

#include <stdio.h>
#include <assert.h>

#define TODO() __debugbreak()

struct Globals {
	HT_TabClass* my_tab_class;
};

// -----------------------------------------------------

static Globals GLOBALS;

// -----------------------------------------------------

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	GLOBALS.my_tab_class = ht->CreateTabClass("Scene Edit");
	
	ht->LogInfo("The\nlazy\ndog!\n");
	ht->LogInfo("The quick brown fox jumped over the lazy dog!\n");
	ht->LogInfo("The quick brown fox jumped over the lazy dog!\n");
	ht->LogWarning("AAAAAAAAAAAAAAAAA!\n");
	ht->LogError("This is a critical error!\n");
	ht->LogError("This is a critical error!\n");
	ht->LogError("This is a critical error!\n");
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	ht->DestroyTabClass(GLOBALS.my_tab_class);
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
}

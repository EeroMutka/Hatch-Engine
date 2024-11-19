#include <hatch_api.h>
#include "../SceneEdit.inc.ht"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

#include <stdio.h>

#define ASSERT(x) do { if (!(x)) __debugbreak(); } while(0)
#define TODO() __debugbreak()

struct Globals {
	HT_TabClass* my_tab_class;
};

// -----------------------------------------------------

static Globals GLOBALS;

// -----------------------------------------------------

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	SceneEditParams* params = HT_GetPluginData(SceneEditParams, ht);
	ASSERT(params);

	// hmm... so maybe SceneEdit can provide an API for unlocking the asset viewer registration.
	// That way, by default it can take the asset viewer, but it also specifies that others can take it if they want to.
	bool ok = ht->RegisterAssetViewerForType(params->scene_type);
	ASSERT(ok);
	
	/*ht->LogInfo("The\nlazy\ndog!\n");
	ht->LogInfo("The quick brown fox jumped over the lazy dog!\n");
	ht->LogInfo("The quick brown fox jumped over the lazy dog!\n");
	ht->LogWarning("AAAAAAAAAAAAAAAAA!\n");
	ht->LogError("This is a critical error!\n");
	ht->LogError("This is a critical error!\n");
	ht->LogError("This is a critical error!\n");*/
	// ht->LogInfo("SceneEdit: Loading plugin OK!");
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// how should the rendering work?
	
	// Scene-edit plugin:
	// 1. take input, update scene
	// 2. render a texture with a placeholder ID as an UI element
	// 3. render gizmos as UI elements
	
	// OR: maybe the scene edit plugin can act more like a library for the renderer plugin.
	// The renderer plugin can then call functions from the scene edit plugin, like "UpdateScene" and "GenerateGizmos".
	// This sounds good, because it lets us use our current simplistic hatch setup. Let's do it.
	// THOUGH... if we do it library-style, then the code will be duplicated. Maybe we need a way to expose DLL calls in the future!
	
	// Ideally, the SceneEdit plugin could render a texture in the UI first
}

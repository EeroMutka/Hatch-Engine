#include <hatch_api.h>

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>
#include <ht_utils/input/input.h>

#include "../fg.inc.ht"

// -----------------------------------------------------

struct Globals {
	int _;
};

// -----------------------------------------------------

static Globals GLOBALS;

// -----------------------------------------------------

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	__fg_params_type* params = HT_GetPluginData(__fg_params_type, ht);
	HT_ASSERT(params);
	
	int open_assets_count;
	HT_Asset* open_assets = ht->GetAllOpenAssetsOfType(params->scene_type, &open_assets_count);
	
	for (int asset_i = 0; asset_i < open_assets_count; asset_i++) {
		HT_Asset scene_asset = open_assets[asset_i];
		Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, scene_asset);

		for (HT_ItemGroupEach(&scene->entities, i)) {
			Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);
			
			if (InputIsDown(ht->input_frame, HT_InputKey_Y)) {
				entity->position.x += 0.1f;
			}
			
			if (InputIsDown(ht->input_frame, HT_InputKey_T)) {
				entity->position.x -= 0.1f;
			}
			
			// ht->LogInfo("entity %f %f %f", entity->position.x, entity->position.y, entity->position.z);
		}
	}
}

#define HT_STATIC_PLUGIN_ID JoltPhysics

#include <hatch_api.h>

#include "../JoltPhysics.inc.ht"

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

struct MeshCollisionData {
	int _;
};

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

// ----------------------------------------------

static Allocator temp_allocator_wrapper;
static Allocator heap_allocator_wrapper;
static DS_Arena temp_arena;
static DS_Arena* TEMP;
static DS_Allocator* HEAP;

static bool has_loaded_scene;
static DS_Map(HT_Asset, MeshCollisionData) meshes_data;
// ----------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align){
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->AllocatorProc(ptr, size);
	return data;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	temp_allocator_wrapper = {{TempAllocatorProc}, ht};
	heap_allocator_wrapper = {{HeapAllocatorProc}, ht};
	TEMP = &temp_arena;
	HEAP = (DS_Allocator*)&heap_allocator_wrapper;

	DS_MapInit(&meshes_data, HEAP);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

static void GenerateMeshCollisionData(HT_Asset mesh_asset, MeshCollisionData* result) {
	// add a box collider?
}

static void SimulateScene(HT_API* ht, Scene__Scene* scene) {
	if (!has_loaded_scene) {
		has_loaded_scene = true;
		// generate collision meshes

		for (HT_ItemGroupEach(&scene->entities, entity_i)) {
			Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);

			bool has_physics = false;
			Scene__MeshComponent* mesh_component = NULL;

			for (int comp_i = 0; comp_i < entity->components.count; comp_i++) {
				HT_Any* comp_any = &((HT_Any*)entity->components.data)[comp_i];
				if (comp_any->type.handle == ht->types->JoltPhysics__PhysicsBodyComponent) {
					has_physics = true;
				}
				if (comp_any->type.handle == ht->types->Scene__MeshComponent) {
					mesh_component = (Scene__MeshComponent*)comp_any->data;
				}
			}

			if (has_physics && mesh_component) {
				// load mesh
				MeshCollisionData* coll_data = NULL;
				bool added_new = DS_MapGetOrAddPtr(&meshes_data, mesh_component->mesh, &coll_data);
				if (added_new) {
					*coll_data = {};
					GenerateMeshCollisionData(mesh_component->mesh, coll_data);
				}
			}
		}
	}
	
	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);

		bool has_physics = false;
		
		for (int comp_i = 0; comp_i < entity->components.count; comp_i++) {
			HT_Any* comp_any = &((HT_Any*)entity->components.data)[comp_i];
			
			if (comp_any->type.handle == ht->types->JoltPhysics__PhysicsBodyComponent) {
				JoltPhysics__PhysicsBodyComponent* physics_body = (JoltPhysics__PhysicsBodyComponent*)comp_any->data;
				has_physics = physics_body->enabled;
				break;
			}
		}

		if (has_physics) {
			entity->position.z -= 0.05f;
		}
	}
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// so lets start by having a "simulate" mode, where jolt physics moves all objects down each frame without any collision.

	if (ht->IsSimulating()) {
		int open_scenes_count = 0;
		HT_Asset* open_scenes = ht->GetAllOpenAssetsOfType(ht->types->Scene__Scene, &open_scenes_count);
		for (int i = 0; i < open_scenes_count; i++) {
			Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, open_scenes[i]);
			SimulateScene(ht, scene);
		}
	}

	// is simulate mode?
	// to exit simulate mode, we can just load the project back from the disk right?

	// two approaches:
	// 1. all data and all assets are automatically made part of the simulate mode when simulating. All data is restored back automatically after exiting simulate mode.
	// 2. plugin code is responsible for generating a simulation instance

	// ... What about hotreloading asset data from disk while a simulation is running? That should be possible right?
	// imagine running into a location as a player, then re-exporting a mesh from blender. Yes, when a change to asset data is detected on disk, the runtime data is overwritten with that data automatically.
	// We can persist data from entities that are selected to allow for running simulations etc. The scene editor plugin can do this internally by saving transforms of selected (or otherwise marked) objects and updating the asset data after the simulation has ended. This is better compared to a "persist this asset" hatch API, because it lets us persist only specific fields (transform).

}

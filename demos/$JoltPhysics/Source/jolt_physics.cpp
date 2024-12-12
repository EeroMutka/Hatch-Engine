#define HT_STATIC_PLUGIN_ID JoltPhysics

#include <hatch_api.h>

#include "../JoltPhysics.inc.ht"

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

#include "joltc.h"

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>

struct MeshCollisionData {
	int _;//JPH_BodyID body_id;
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

//static bool has_loaded_scene;
static DS_Map(HT_Asset, MeshCollisionData) meshes_data;

static JPH_JobSystem* jolt_jobSystem;
static JPH_PhysicsSystem* jolt_system;
static JPH_BodyInterface* jolt_bodyInterface;


// ----------------------------------------------

#define FIND_COMPONENT(HT, ENTITY, T) FindComponent<T>(ENTITY, HT->types->T)

template<typename T>
static T* FindComponent(Scene__SceneEntity* entity, HT_Asset struct_type) {
	T* result = NULL;
	for (int i = 0; i < entity->components.count; i++) {
		HT_Any component = ((HT_Any*)entity->components.data)[i];
		if (component.type.handle == struct_type) {
			result = (T*)component.data;
			break;
		}
	}
	return result;
}

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align){
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->AllocatorProc(ptr, size);
	return data;
}

static void JoltTraceHandler(const char* message) {
	HT_LogInfo("JOLT: %s", message);
}

static const JPH_ObjectLayer LAYER_NON_MOVING = 0;
static const JPH_ObjectLayer LAYER_MOVING = 1;
static const JPH_ObjectLayer NUM_LAYERS = 2;

static const JPH_BroadPhaseLayer BROAD_PHASE_LAYER_NON_MOVING = 0;
static const JPH_BroadPhaseLayer BROAD_PHASE_LAYER_MOVING = 1;
static const uint32_t BROAD_PHASE_NUM_LAYERS = 2;

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	temp_allocator_wrapper = {{TempAllocatorProc}, ht};
	heap_allocator_wrapper = {{HeapAllocatorProc}, ht};
	TEMP = &temp_arena;
	HEAP = (DS_Allocator*)&heap_allocator_wrapper;

	DS_MapInit(&meshes_data, HEAP);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

static void GenerateMeshCollisionData(HT_Asset mesh_asset, Scene__SceneEntity* entity, MeshCollisionData* result) {
}

static void StartSimulation(HT_API* ht, Scene__Scene* scene) {
	bool ok = JPH_Init();
	HT_ASSERT(ok);

	JPH_SetTraceHandler(JoltTraceHandler);
	//JPH_SetAssertFailureHandler(JPH_AssertFailureFunc handler);

	jolt_jobSystem = JPH_JobSystemThreadPool_Create(NULL);

	JPH_ObjectLayerPairFilter* objectLayerPairFilterTable;
	JPH_BroadPhaseLayerInterface* broadPhaseLayerInterfaceTable;
	JPH_ObjectVsBroadPhaseLayerFilter* objectVsBroadPhaseLayerFilter;

	// We use only 2 layers: one for non-moving objects and one for moving objects
	objectLayerPairFilterTable = JPH_ObjectLayerPairFilterTable_Create(2);
	JPH_ObjectLayerPairFilterTable_EnableCollision(objectLayerPairFilterTable, LAYER_NON_MOVING, LAYER_MOVING);
	JPH_ObjectLayerPairFilterTable_EnableCollision(objectLayerPairFilterTable, LAYER_MOVING, LAYER_NON_MOVING);
	JPH_ObjectLayerPairFilterTable_EnableCollision(objectLayerPairFilterTable, LAYER_MOVING, LAYER_MOVING);

	// We use a 1-to-1 mapping between object layers and broadphase layers
	broadPhaseLayerInterfaceTable = JPH_BroadPhaseLayerInterfaceTable_Create(2, 2);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(broadPhaseLayerInterfaceTable, LAYER_NON_MOVING, BROAD_PHASE_LAYER_NON_MOVING);
	JPH_BroadPhaseLayerInterfaceTable_MapObjectToBroadPhaseLayer(broadPhaseLayerInterfaceTable, LAYER_MOVING, BROAD_PHASE_LAYER_MOVING);

	objectVsBroadPhaseLayerFilter = JPH_ObjectVsBroadPhaseLayerFilterTable_Create(broadPhaseLayerInterfaceTable, 2, objectLayerPairFilterTable, 2);

	JPH_PhysicsSystemSettings settings = {};
	settings.maxBodies = 65536;
	settings.numBodyMutexes = 0;
	settings.maxBodyPairs = 65536;
	settings.maxContactConstraints = 65536;
	settings.broadPhaseLayerInterface = broadPhaseLayerInterfaceTable;
	settings.objectLayerPairFilter = objectLayerPairFilterTable;
	settings.objectVsBroadPhaseLayerFilter = objectVsBroadPhaseLayerFilter;
	jolt_system = JPH_PhysicsSystem_Create(&settings);

	JPH_Vec3 gravity = {0.f, 0.f, -9.81f * 0.1f};
	JPH_PhysicsSystem_SetGravity(jolt_system, &gravity);

	jolt_bodyInterface = JPH_PhysicsSystem_GetBodyInterface(jolt_system);

	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);
		Scene__MeshComponent* mesh_component = FIND_COMPONENT(ht, entity, Scene__MeshComponent);
		JoltPhysics__PhysicsComponent* body_component = FIND_COMPONENT(ht, entity, JoltPhysics__PhysicsComponent);
		JoltPhysics__BoxCollisionComponent* box_collision_component = FIND_COMPONENT(ht, entity, JoltPhysics__BoxCollisionComponent);

		if (body_component) {
			// load mesh
			//MeshCollisionData* coll_data = NULL;
			//bool added_new = DS_MapGetOrAddPtr(&meshes_data, mesh_component->mesh, &coll_data);
			//if (added_new) {
			//	*coll_data = {};
			//	GenerateMeshCollisionData(mesh_component->mesh, entity, coll_data);
			//}

			JPH_Shape* shape = NULL;
//			if (box_collision_component) {
				JPH_RVec3 half_extent = { 0.5f*entity->scale.x, 0.5f*entity->scale.y, 0.5f*entity->scale.z };
				shape = (JPH_Shape*)JPH_BoxShape_Create(&half_extent, 0.f);
			//}
			//else {
			//	shape = (JPH_Shape*)JPH_SphereShape_Create(0.5f);
			//}
				
			JPH_RVec3 position = { entity->position.x, entity->position.y, entity->position.z };
			JPH_BodyCreationSettings* body_settings = JPH_BodyCreationSettings_Create3(
				shape,
				&position,
				NULL, // Identity, 
				body_component->dynamic ? JPH_MotionType_Dynamic : JPH_MotionType_Static,
				body_component->dynamic ? LAYER_MOVING : LAYER_NON_MOVING);

			JPH_BodyID body_id = JPH_BodyInterface_CreateAndAddBody(jolt_bodyInterface, body_settings, body_component->dynamic ? JPH_Activation_Activate : JPH_Activation_DontActivate);
			JPH_BodyCreationSettings_Destroy(body_settings);

			// Now you can interact with the dynamic body, in this case we're going to give it a velocity.
			// (note that if we had used CreateBody then we could have set the velocity straight on the body before adding it to the physics system)
			JPH_RVec3 sphereLinearVelocity = { 0.0f, 0.0f, 0.0f };
			JPH_BodyInterface_SetLinearVelocity(jolt_bodyInterface, body_id, &sphereLinearVelocity);
			body_component->jph_body_id = body_id;
		}
	}
}

static void EndSimulation(HT_API* ht, Scene__Scene* scene) {
	JPH_JobSystem_Destroy(jolt_jobSystem);
	JPH_PhysicsSystem_Destroy(jolt_system);
	JPH_Shutdown();
	jolt_jobSystem = NULL;
	jolt_system = NULL;
	jolt_bodyInterface = NULL;
}

static void QuatToEulerAngles(JPH_Quat q, float* roll, float* pitch, float* yaw) {
	// see https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
	
	// roll (x-axis rotation)
	float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	*roll = atan2f(sinr_cosp, cosr_cosp) * M_RadToDeg;

	// pitch (y-axis rotation)
	float sinp = sqrtf(1 + 2 * (q.w * q.y - q.x * q.z));
	float cosp = sqrtf(1 - 2 * (q.w * q.y - q.x * q.z));
	*pitch = (2 * atan2f(sinp, cosp) - M_PI / 2) * M_RadToDeg;

	// yaw (z-axis rotation)
	float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	*yaw = atan2f(siny_cosp, cosy_cosp) * M_RadToDeg;
}

static void SimulateScene(HT_API* ht, Scene__Scene* scene) {
	// We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
	const float cDeltaTime = 1.0f / 60.0f;

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	// JPH_PhysicsSystem_OptimizeBroadPhase(system);

	JPH_PhysicsSystem_Update(jolt_system, cDeltaTime, 1, jolt_jobSystem);

	for (HT_ItemGroupEach(&scene->entities, entity_i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, entity_i);
		
		JoltPhysics__PhysicsComponent* body = FIND_COMPONENT(ht, entity, JoltPhysics__PhysicsComponent);
		if (body && body->enabled) {
			//JPH_BodyInterface_GetLinearVelocity(bodyInterface, sphereId, &velocity);
			
			JPH_RVec3 p;
			JPH_BodyInterface_GetCenterOfMassPosition(jolt_bodyInterface, body->jph_body_id, &p);

			JPH_Quat q;
			JPH_BodyInterface_GetRotation(jolt_bodyInterface, body->jph_body_id, &q);
			
			QuatToEulerAngles(q, &entity->rotation.x, &entity->rotation.y, &entity->rotation.z);

			entity->position = {p.x, p.y, p.z};
		}
	}
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// so lets start by having a "simulate" mode, where jolt physics moves all objects down each frame without any collision.

	static bool was_simulating = false;

	Scene__Scene* scene = NULL;
	int open_scenes_count = 0;
	HT_Asset* open_scenes = ht->GetAllOpenAssetsOfType(ht->types->Scene__Scene, &open_scenes_count);
	for (int i = 0; i < open_scenes_count; i++) {
		scene = HT_GetAssetData(Scene__Scene, ht, open_scenes[i]);
	}

	bool is_simulating = ht->IsSimulating();
	if (is_simulating && !was_simulating) {
		StartSimulation(ht, scene);
	}
	if (is_simulating) {
		SimulateScene(ht, scene);
	}
	if (!is_simulating && was_simulating) {
		EndSimulation(ht, scene);
	}
	was_simulating = is_simulating;
	// is simulate mode?
	// to exit simulate mode, we can just load the project back from the disk right?

	// two approaches:
	// 1. all data and all assets are automatically made part of the simulate mode when simulating. All data is restored back automatically after exiting simulate mode.
	// 2. plugin code is responsible for generating a simulation instance

	// ... What about hotreloading asset data from disk while a simulation is running? That should be possible right?
	// imagine running into a location as a player, then re-exporting a mesh from blender. Yes, when a change to asset data is detected on disk, the runtime data is overwritten with that data automatically.
	// We can persist data from entities that are selected to allow for running simulations etc. The scene editor plugin can do this internally by saving transforms of selected (or otherwise marked) objects and updating the asset data after the simulation has ended. This is better compared to a "persist this asset" hatch API, because it lets us persist only specific fields (transform).

}



//JPH_BodyID floorId = {};
//{
//	// Next we can create a rigid body to serve as the floor, we make a large box
//	// Create the settings for the collision volume (the shape). 
//	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
//	JPH_Vec3 boxHalfExtents = { 100.0f, 1.0f, 100.0f };
//	JPH_BoxShape* floorShape = JPH_BoxShape_Create(&boxHalfExtents, JPH_DEFAULT_CONVEX_RADIUS);
//
//	JPH_Vec3 floorPosition = { 0.0f, -1.0f, 0.0f };
//	JPH_BodyCreationSettings* floorSettings = JPH_BodyCreationSettings_Create3(
//		(const JPH_Shape*)floorShape,
//		&floorPosition,
//		NULL, // Identity, 
//		JPH_MotionType_Static,
//		LAYER_NON_MOVING);
//
//	// Create the actual rigid body
//	floorId = JPH_BodyInterface_CreateAndAddBody(bodyInterface, floorSettings, JPH_Activation_DontActivate);
//	JPH_BodyCreationSettings_Destroy(floorSettings);
//}

//{
//	static constexpr float	cCharacterHeightStanding = 1.35f;
//	static constexpr float	cCharacterRadiusStanding = 0.3f;
//	static constexpr float	cCharacterHeightCrouching = 0.8f;
//	static constexpr float	cCharacterRadiusCrouching = 0.3f;
//	static constexpr float	cInnerShapeFraction = 0.9f;
//
//	JPH_CapsuleShape* capsuleShape = JPH_CapsuleShape_Create(0.5f * cCharacterHeightStanding, cCharacterRadiusStanding);
//	JPH_Vec3 position = { 0, 0.5f * cCharacterHeightStanding + cCharacterRadiusStanding, 0 };
//	auto mStandingShape = JPH_RotatedTranslatedShape_Create(&position, NULL, (JPH_Shape*)capsuleShape);
//
//	JPH_CharacterVirtualSettings characterSettings{};
//	JPH_CharacterVirtualSettings_Init(&characterSettings);
//	characterSettings.base.shape = (const JPH_Shape*)mStandingShape;
//	characterSettings.base.supportingVolume = { {0, 1, 0}, -cCharacterRadiusStanding }; // Accept contacts that touch the lower sphere of the capsule
//	static const JPH_RVec3 characterVirtualPosition = { -5.0f, 0, 3.0f };
//
//	auto mAnimatedCharacterVirtual = JPH_CharacterVirtual_Create(&characterSettings, &characterVirtualPosition, NULL, 0, system);
//}

// Remove the destroy sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
//JPH_BodyInterface_RemoveAndDestroyBody(bodyInterface, sphereId);
//
//// Remove and destroy the floor
//JPH_BodyInterface_RemoveAndDestroyBody(bodyInterface, floorId);
//
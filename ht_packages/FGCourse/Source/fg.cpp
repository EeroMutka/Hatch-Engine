#define HT_STATIC_PLUGIN_ID fg
#define FIRE_OS_SYNC_IMPLEMENTATION

#include "common.h"
#include "message_manager.h"
#include "render_manager.h"
#include "mesh_manager.h"
#include "audio_manager.h"

#include <ht_utils/gizmos/gizmos.h>

#include "ht_packages/SceneEdit/src/camera.h"
#include "ht_packages/SceneEdit/src/scene_edit.h"

// -----------------------------------------------------

HT_API* FG::ht;
DS_BasicMemConfig FG::mem;
static SceneEditState scene_edit_state;

// -----------------------------------------------------

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = FG::ht->AllocatorProc(ptr, size);
	return data;
}

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

static void AssetViewerTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);

	if (update_info->drag_n_dropped_asset)
	{
		//HT_ItemGroup
		HT_ItemIndex new_index = ht->ItemGroupAdd(&scene->entities);
		ht->MoveItemToAfter(&scene->entities, new_index, 0);
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, new_index);
		*entity = {};
	}

	vec2 mouse_pos = ht->input_frame->mouse_position;
	vec2 rect_size = update_info->rect.max - update_info->rect.min;

	mat4 world_to_clip = {};

	SceneEdit__EditorCamera* camera = NULL;
	for (int i = 0; i < scene->extended_data.count; i++) {
		HT_Any any = ((HT_Any*)scene->extended_data.data)[i];
		if (any.type.handle == ht->types->SceneEdit__EditorCamera) {
			camera = (SceneEdit__EditorCamera*)any.data;

			// TODO: I think we should encode the FOV and the `ws_to_vs` matrix and the near/far values in SceneEdit__EditorCamera
			mat4 ws_to_vs =
				M_MatTranslate(-1.f*camera->position) *
				M_MatRotateZ(-camera->yaw) *
				M_MatRotateY(-camera->pitch) *
				mat4{
					0.f,  0.f,  1.f, 0.f,
					-1.f,  0.f,  0.f, 0.f,
					0.f, -1.f,  0.f, 0.f,
					0.f,  0.f,  0.f, 1.f,
				};
			
			if (camera->orthographic)
			{
				world_to_clip = ws_to_vs * M_MatScale(vec3{1.f / camera->fov, 1.f / camera->fov, 0.001f});
			}
			else
			{
				world_to_clip = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*camera->fov, (float)rect_size.x / (float)rect_size.y, 0.1f, 1000.f);
			}
			break;
		}
	}

	vec2 rect_middle = (update_info->rect.min + update_info->rect.max) * 0.5f;
	mat4 cs_to_ss = M_MatScale(vec3{0.5f*rect_size.x, 0.5f*rect_size.y, 1.f}) * M_MatTranslate(vec3{rect_middle.x, rect_middle.y, 0});

	M_PerspectiveView open_scene_view = {};
	open_scene_view.has_camera_position = true;
	open_scene_view.camera_position = camera->position;
	open_scene_view.ws_to_ss = world_to_clip * cs_to_ss;
	open_scene_view.ss_to_ws = M_Inverse4x4(open_scene_view.ws_to_ss);
	
	SceneEditUpdate(ht, &scene_edit_state, update_info->rect, mouse_pos, scene);

	for (HT_ItemGroupEach(&scene->entities, i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);

		mat4 local_to_world =
			M_MatScale(entity->scale) *
			M_MatRotateX(entity->rotation.x * M_DegToRad) *
			M_MatRotateY(entity->rotation.y * M_DegToRad) *
			M_MatRotateZ(entity->rotation.z * M_DegToRad) *
			M_MatTranslate(entity->position);

		Scene__MeshComponent* mesh_component = FIND_COMPONENT(ht, entity, Scene__MeshComponent);
		if (mesh_component) {

			RenderMesh* mesh_data = MeshManager::GetMeshFromMeshAsset(mesh_component->mesh);
			RenderTexture* color_texture = MeshManager::GetTextureFromTextureAsset(mesh_component->color_texture, RenderTextureFormat::RGBA8);
			
			RenderTexture* specular_texture = mesh_component->specular_texture ?
				MeshManager::GetTextureFromTextureAsset(mesh_component->specular_texture, RenderTextureFormat::R8) : NULL;

			if (mesh_data && color_texture) {
				RenderObjectMessage msg = {};
				msg.mesh = mesh_data;
				msg.color_texture = color_texture;
				msg.specular_texture = specular_texture;
				msg.specular_value = mesh_component->specular_value;
				msg.local_to_world = local_to_world;
				MessageManager::SendNewMessage(msg);
			}
		}

		Scene__PointLightComponent* point_light_component = FIND_COMPONENT(ht, entity, Scene__PointLightComponent);
		if (point_light_component) {
			AddPointLightMessage msg = {};
			msg.position = entity->position;
			msg.emission = point_light_component->emission;
			MessageManager::SendNewMessage(msg);
		}
		
		Scene__DirectionalLightComponent* dir_light_component = FIND_COMPONENT(ht, entity, Scene__DirectionalLightComponent);
		if (dir_light_component) {
			AddDirectionalLightMessage msg = {};
			msg.rotation = entity->rotation;
			msg.emission = dir_light_component->emission;
			MessageManager::SendNewMessage(msg);
		}

		Scene__SpotLightComponent* spot_light_component = FIND_COMPONENT(ht, entity, Scene__SpotLightComponent);
		if (spot_light_component) {
			AddSpotLightMessage msg = {};
			msg.position = entity->position;
			msg.direction = local_to_world.row[2].xyz * -1.f;
			msg.emission = spot_light_component->emission;
			MessageManager::SendNewMessage(msg);
		}
	}

	RenderParamsMessage render_params_msg = {};
	render_params_msg.rect = update_info->rect;
	render_params_msg.world_to_clip = world_to_clip;
	render_params_msg.view_position = camera->position;
	render_params_msg.scene = scene;
	render_params_msg.scene_edit_state = &scene_edit_state;
	MessageManager::SendNewMessage(render_params_msg);
}

void FG::Init(HT_API* ht_api) {
	ht = ht_api;

	// Copied from DS_InitBasicMemConfig
	mem.ds_info.temp_arena = &mem.temp_arena;
	mem.heap_allocator.allocator_proc = HeapAllocatorProc;
	mem.heap_allocator.ds = &mem.ds_info;
	DS_ArenaInit(&mem.temp_arena, 4096, (DS_Allocator*)&mem.heap_allocator);	
	mem.temp = &mem.temp_arena;
	mem.ds = &mem.ds_info;
	mem.heap = (DS_Allocator*)&mem.heap_allocator;
}

void FG::ResetTempArena() {
	DS_ArenaReset(&mem.temp_arena);
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	FG::Init(ht);
	MessageManager::Init();
	MeshManager::Init();
	RenderManager::Init();
	AudioManager::Init();
	
	MessageManager::RegisterNewThread();

	bool ok = ht->RegisterAssetViewerForType(ht->types->Scene__Scene, AssetViewerTabUpdate);
	HT_ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

struct MyMessageA : Message {
	int x;
	int y;
};

struct MyMessageB : Message {
	float number;
};

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	FG::ResetTempArena();
	MessageManager::BeginFrame();

	if (InputIsDown(ht->input_frame, HT_InputKey_P)) {
		PlaySoundMessage play_sound = {};
		play_sound.add_pitch = 20.f;
		MessageManager::SendNewMessage(play_sound);
	}

	if (InputIsDown(ht->input_frame, HT_InputKey_O)) {
		PlaySoundMessage play_sound = {};
		play_sound.add_pitch = -5.f;
		MessageManager::SendNewMessage(play_sound);
	}

	if (InputIsDown(ht->input_frame, HT_InputKey_I)) {
		PlaySoundMessage play_sound = {};
		play_sound.add_pitch = -2.f;
		MessageManager::SendNewMessage(play_sound);
	}

	if (InputIsDown(ht->input_frame, HT_InputKey_L)) {
		PlaySoundMessage play_sound = {};
		play_sound.add_pitch = 10.f;
		MessageManager::SendNewMessage(play_sound);
	}

	MessageManager::ThreadBarrierAllMessagesSent();
}

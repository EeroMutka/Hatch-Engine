#define HT_STATIC_PLUGIN_ID fg


#include "common.h"
#include "message_manager.h"
#include "mesh_manager.h"
#include "render_manager.h"

#include <ht_utils/gizmos/gizmos.h>

#include "../../$SceneEdit/src/camera.h"
#include "../../$SceneEdit/src/scene_edit.h"

// -----------------------------------------------------

// -----------------------------------------------------

DS_Arena* FG::temp;
DS_Allocator* FG::heap;
HT_API* FG::ht;
Allocator FG::temp_allocator_wrapper;
Allocator FG::heap_allocator_wrapper;
DS_Arena FG::temp_arena;;


static Scene__Scene* open_scene;
static M_PerspectiveView open_scene_view;
static mat4 open_scene_ws_to_cs;
static ivec2 open_scene_rect_min;
static ivec2 open_scene_rect_max;
static SceneEditState scene_edit_state;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align){
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->AllocatorProc(ptr, size);
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

static void Render(HT_API* ht) {
	RenderManager::RenderAllQueued();
}

static void AssetViewerTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	open_scene = scene;
	open_scene_rect_min = update_info->rect_min;
	open_scene_rect_max = update_info->rect_max;

	vec2 mouse_pos = ht->input_frame->mouse_position;// - vec2{(float)open_scene_rect_min.x, (float)open_scene_rect_min.y};

	int width = open_scene_rect_max.x - open_scene_rect_min.x;
	int height = open_scene_rect_max.y - open_scene_rect_min.y;

	open_scene_ws_to_cs = {};

	SceneEdit__EditorCamera* camera = NULL;
	for (int i = 0; i < open_scene->extended_data.count; i++) {
		HT_Any any = ((HT_Any*)open_scene->extended_data.data)[i];
		if (any.type.handle == ht->types->SceneEdit__EditorCamera) {
			camera = (SceneEdit__EditorCamera*)any.data;

			//open_scene_ws_to_cs = M_MatScale({0.1f, 0.1f, 0.1f});
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
			open_scene_ws_to_cs = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, (float)width / (float)height, 0.1f, 1000.f);
			break;
		}
	}

	vec2 rect_size = { (float)width, (float)height };
	vec2 rect_middle = (vec2{(float)open_scene_rect_min.x, (float)open_scene_rect_min.y} + vec2{(float)open_scene_rect_max.x, (float)open_scene_rect_max.y}) * 0.5f;
	mat4 cs_to_ss = M_MatScale(vec3{0.5f*rect_size.x, 0.5f*rect_size.y, 1.f}) * M_MatTranslate(vec3{rect_middle.x, rect_middle.y, 0});

	open_scene_view = {};
	open_scene_view.has_camera_position = true;
	open_scene_view.camera_position = camera->position;
	open_scene_view.ws_to_ss = open_scene_ws_to_cs * cs_to_ss;
	invert4x4((float*)&open_scene_view.ws_to_ss, (float*)&open_scene_view.ss_to_ws);
	
	//open_scene_view.ss_to_ws = M_Inverse4x4(open_scene_view.ws_to_ss);

	//ht->LogInfo("mouse_pos %f %f\n", mouse_pos.x, mouse_pos.y);

	SceneEditUpdate(ht, &scene_edit_state, &open_scene_view, mouse_pos, scene);

	for (HT_ItemGroupEach(&scene->entities, i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);

		Scene__MeshComponent* mesh_component = FIND_COMPONENT(ht, entity, Scene__MeshComponent);
		if (mesh_component) {
			mat4 local_to_world =
				M_MatScale(entity->scale) *
				M_MatRotateX(entity->rotation.x * M_DegToRad) *
				M_MatRotateY(entity->rotation.y * M_DegToRad) *
				M_MatRotateZ(entity->rotation.z * M_DegToRad) *
				M_MatTranslate(entity->position);

			RenderObjectMessage render_object_msg = {};
			render_object_msg.mesh = mesh_component->mesh;
			render_object_msg.color_texture = mesh_component->color_texture;
			render_object_msg.local_to_world = local_to_world;
			MessageManager::SendNewMessage(render_object_msg);
		}

		if (InputIsDown(ht->input_frame, HT_InputKey_Y)) {
			entity->position.x += 0.1f;
		}

		if (InputIsDown(ht->input_frame, HT_InputKey_T)) {
			entity->position.x -= 0.1f;
		}
	}
}

void FG::Init(HT_API* ht_api) {
	ht = ht_api;
	temp_allocator_wrapper = {{TempAllocatorProc}, ht};
	heap_allocator_wrapper = {{HeapAllocatorProc}, ht};
	temp = &temp_arena;
	heap = (DS_Allocator*)&heap_allocator_wrapper;
}

void FG::ResetTempArena() {
	DS_ArenaInit(&temp_arena, 0, (DS_Allocator*)&temp_allocator_wrapper);
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	FG::Init(ht);
	MessageManager::Init();
	MeshManager::Init();
	RenderManager::Init();
	
	// create cbo
	D3D11_BUFFER_DESC cbo_desc = {};
	cbo_desc.ByteWidth      = sizeof(ShaderConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	cbo_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cbo_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbo_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ht->D3D11_device->CreateBuffer(&cbo_desc, nullptr, &cbo);
	
	// create sampler
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ht->D3D11_device->CreateSamplerState(&sampler_desc, &sampler);

	HT_StringView main_shader_path = HATCH_DIR "/demos/$FGCourse/Shaders/main_shader.hlsl";
	HT_StringView present_shader_path = HATCH_DIR "/demos/$FGCourse/Shaders/present_shader.hlsl";

	D3D11_INPUT_ELEMENT_DESC main_vs_inputs[] = {
		{    "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{     "UV", 0,    DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	main_vs = LoadVertexShader(ht, main_shader_path, main_vs_inputs, _countof(main_vs_inputs));
	main_ps = LoadPixelShader(ht, main_shader_path);

	present_vs = LoadVertexShader(ht, present_shader_path, NULL, 0);
	present_ps = LoadPixelShader(ht, present_shader_path);

	ht->D3D11_SetRenderProc(Render);

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

	/*
	MyMessageA send_1;
	send_1.x = 50;
	send_1.y = 40;
	MessageManager::SendNewMessage(send_1);

	MyMessageB send_2;
	send_2.number = 54321.f;
	MessageManager::SendNewMessage(send_2);

	MyMessageA send_3;
	send_3.x = 400;
	send_3.y = 900;
	MessageManager::SendNewMessage(send_3);

	// -----

	MyMessageB get_b1;
	if (MessageManager::PopNextMessage(&get_b1)) {
		__debugbreak();
	}

	MyMessageB get_b2;
	if (MessageManager::PopNextMessage(&get_b2)) {
		__debugbreak();
	}

	MyMessageA get_a1;
	if (MessageManager::PopNextMessage(&get_a1)) {
		__debugbreak();
	}

	MyMessageA get_a2;
	if (MessageManager::PopNextMessage(&get_a2)) {
		__debugbreak();
	}
	__debugbreak();*/
}

#define HT_STATIC_PLUGIN_ID fg


#include "common.h"
#include "message_manager.h"
#include "mesh_manager.h"

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>
#include <ht_utils/gizmos/gizmos.h>

#include "../../$SceneEdit/src/camera.h"
#include "../../$SceneEdit/src/scene_edit.h"

// -----------------------------------------------------

struct ShaderConstants {
	mat4 ws_to_cs;
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* input_layout;
};

struct UIBackend {
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

// -----------------------------------------------------

DS_Arena* FG::temp;
DS_Allocator* FG::heap;
HT_API* FG::ht;
Allocator FG::temp_allocator_wrapper;
Allocator FG::heap_allocator_wrapper;
DS_Arena FG::temp_arena;;

static ID3D11Buffer* cbo;
static ID3D11SamplerState* sampler;

static VertexShader main_vs;
static ID3D11PixelShader* main_ps;

static VertexShader present_vs;
static ID3D11PixelShader* present_ps;

static ID3D11InputLayout* input_layout;

static Scene__Scene* open_scene;
static ivec2 open_scene_rect_min;
static ivec2 open_scene_rect_max;

static ID3D11Texture2D* color_target;
static ID3D11Texture2D* depth_target;
static ID3D11ShaderResourceView* color_target_srv;
static ID3D11RenderTargetView* color_target_view;
static ID3D11DepthStencilView* depth_target_view;

static UIBackend G_UI;

// -----------------------------------------------------

static UI_CachedGlyph UIGetCachedGlyph(u32 codepoint, UI_Font font) {
	HT_CachedGlyph glyph = FG::ht->GetCachedGlyph(codepoint);
	return *(UI_CachedGlyph*)&glyph;
}

static UI_DrawVertex* UIResizeAndMapVertexBuffer(int num_vertices) {
	DS_ArrResizeUndef(&G_UI.vertex_buffer, num_vertices);
	return G_UI.vertex_buffer.data;
}

static u32* UIResizeAndMapIndexBuffer(int num_indices) {
	DS_ArrResizeUndef(&G_UI.index_buffer, num_indices);
	return G_UI.index_buffer.data;
}

// -----------------------------------------------------

void MeshManager::Init() {
	DS_MapInit(&instance.meshes, FG::heap);
	DS_MapInit(&instance.textures, FG::heap);
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

static void MaybeRecreateRenderTargets(HT_API* ht, int width, int height) {
	D3D11_TEXTURE2D_DESC desc;
	if (color_target) color_target->GetDesc(&desc);
	if (color_target == NULL || desc.Width != width || desc.Height != height) {
		if (color_target) {
			color_target->Release();
			color_target_view->Release();
			color_target_srv->Release();
			depth_target->Release();
			depth_target_view->Release();
		}

		desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		bool ok = ht->D3D11_device->CreateTexture2D(&desc, NULL, &color_target) == S_OK;
		HT_ASSERT(ok);

		ok = ht->D3D11_device->CreateRenderTargetView(color_target, NULL, &color_target_view) == S_OK;
		HT_ASSERT(ok);

		ok = ht->D3D11_device->CreateShaderResourceView(color_target, NULL, &color_target_srv) == S_OK;
		HT_ASSERT(ok);
		
		D3D11_TEXTURE2D_DESC depth_desc = {};
		depth_desc.Width = width;
		depth_desc.Height = height;
		depth_desc.MipLevels = 1;
		depth_desc.ArraySize = 1;
		depth_desc.SampleDesc.Count = 1;
		depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ok = ht->D3D11_device->CreateTexture2D(&depth_desc, NULL, &depth_target) == S_OK;
		HT_ASSERT(ok);

		ok = ht->D3D11_device->CreateDepthStencilView(depth_target, NULL, &depth_target_view) == S_OK;
		HT_ASSERT(ok);
	}
}

static void UpdateShaderConstants(ID3D11DeviceContext* dc, ShaderConstants val) {
	D3D11_MAPPED_SUBRESOURCE cbo_mapped;
	dc->Map(cbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbo_mapped);
	*(ShaderConstants*)cbo_mapped.pData = val;
	dc->Unmap(cbo, 0);
}

static void Render(HT_API* ht) {
	if (open_scene) {
		
		// setup UI / gizmos rendering
		{
			DS_ArrInit(&G_UI.vertex_buffer, FG::temp);
			DS_ArrInit(&G_UI.index_buffer, FG::temp);
			UI_STATE.backend.ResizeAndMapVertexBuffer = UIResizeAndMapVertexBuffer;
			UI_STATE.backend.ResizeAndMapIndexBuffer  = UIResizeAndMapIndexBuffer;
			
			// setup UI text rendering backend
			UI_STATE.backend.GetCachedGlyph = UIGetCachedGlyph;
			
			UI_TEMP = FG::temp;
			UI_ResetDrawState();
		}
	
		int width = open_scene_rect_max.x - open_scene_rect_min.x;
		int height = open_scene_rect_max.y - open_scene_rect_min.y;
		MaybeRecreateRenderTargets(ht, width, height);

		mat4 ws_to_cs = {};
		for (int i = 0; i < open_scene->extended_data.count; i++) {
			HT_Any any = ((HT_Any*)open_scene->extended_data.data)[i];
			if (any.type.handle == ht->types->SceneEdit__EditorCamera) {
				SceneEdit__EditorCamera* camera = (SceneEdit__EditorCamera*)any.data;

				//ws_to_cs = M_MatScale({0.1f, 0.1f, 0.1f});
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
				ws_to_cs = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, (float)width / (float)height, 0.1f, 100.f);
			}
		}

		vec2 rect_size = { (float)width, (float)height };
		vec2 rect_middle = (vec2{(float)open_scene_rect_min.x, (float)open_scene_rect_min.y} + vec2{(float)open_scene_rect_max.x, (float)open_scene_rect_max.y}) * 0.5f;
		mat4 cs_to_ss = M_MatScale(vec3{0.5f*rect_size.x, 0.5f*rect_size.y, 1.f}) * M_MatTranslate(vec3{rect_middle.x, rect_middle.y, 0});

		M_PerspectiveView view = {};
		view.position = {0, 0, 0};
		view.ws_to_ss = ws_to_cs * cs_to_ss;
		SceneEditDrawGizmos(&view);

		ID3D11DeviceContext* dc = ht->D3D11_device_context;

		ShaderConstants constants = {};
		constants.ws_to_cs = ws_to_cs;
		UpdateShaderConstants(dc, constants);

		dc->PSSetSamplers(0, 1, &sampler);

		dc->ClearDepthStencilView(depth_target_view, D3D11_CLEAR_DEPTH, 1.0f, 0);		

		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->IASetInputLayout(main_vs.input_layout);
		dc->VSSetShader(main_vs.shader, NULL, 0);
		dc->VSSetConstantBuffers(0, 1, &cbo);
		dc->PSSetShader(main_ps, NULL, 0);

		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = (float)width;
		viewport.Height = (float)height;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		dc->RSSetViewports(1, &viewport);

		// keep from hatch
		// dc->OMSetBlendState(s->blend_state, NULL, 0xffffffff);

		///////////////////////////////////////////////////////////////////////////////////////////

		float clear_color[] = {0.2f, 0.4f, 0.8f, 1.f};
		dc->ClearRenderTargetView(color_target_view, clear_color);
		dc->OMSetRenderTargets(1, &color_target_view, depth_target_view);
		
		for (HT_ItemGroupEach(&open_scene->entities, entity_i)) {
			Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &open_scene->entities, entity_i);
			Scene__MeshComponent* mesh_component = FIND_COMPONENT(ht, entity, Scene__MeshComponent);
			
			mat4 ls_to_ws =
				M_MatScale(entity->scale) *
				M_MatRotateX(entity->rotation.x * M_DegToRad) *
				M_MatRotateY(entity->rotation.y * M_DegToRad) *
				M_MatRotateZ(entity->rotation.z * M_DegToRad) *
				M_MatTranslate(entity->position);

			constants.ws_to_cs = ls_to_ws * ws_to_cs;
			UpdateShaderConstants(dc, constants);

			if (mesh_component) {
				Mesh mesh_data;
				Texture color_texture;
				bool ok =
					MeshManager::GetMeshFromMeshAsset(mesh_component->mesh, &mesh_data) &&
					MeshManager::GetColorTextureFromTextureAsset(mesh_component->color_texture, &color_texture);
				
				if (ok) {
					for (int i = 0; i < mesh_data.parts.count; i++) {
						MeshPart part = mesh_data.parts[i];
						u32 vbo_offset = 0;
						u32 vbo_stride = sizeof(Vertex);
						dc->IASetVertexBuffers(0, 1, &mesh_data.vbo, &vbo_stride, &vbo_offset);
						dc->IASetIndexBuffer(mesh_data.ibo, DXGI_FORMAT_R32_UINT, 0);

						dc->PSSetShaderResources(0, 1, &color_texture.texture_srv);
						dc->DrawIndexed(part.index_count, part.base_index_location, part.base_vertex_location);
					}
				}
			}

			if (InputIsDown(ht->input_frame, HT_InputKey_Y)) {
				entity->position.x += 0.1f;
			}

			if (InputIsDown(ht->input_frame, HT_InputKey_T)) {
				entity->position.x -= 0.1f;
			}
		}

		{
			D3D11_VIEWPORT present_viewport = {};
			present_viewport.TopLeftX = (float)open_scene_rect_min.x;
			present_viewport.TopLeftY = (float)open_scene_rect_min.y;
			present_viewport.Width = (float)width;
			present_viewport.Height = (float)height;
			present_viewport.MinDepth = 0.f;
			present_viewport.MaxDepth = 1.f;
			dc->RSSetViewports(1, &present_viewport);

			ID3D11RenderTargetView* present_target = ht->D3D11_GetHatchRenderTargetView();
		
			dc->VSSetShader(present_vs.shader, NULL, 0);
			dc->PSSetShader(present_ps, NULL, 0);
			dc->OMSetRenderTargets(1, &present_target, NULL);

			dc->PSSetShaderResources(0, 1, &color_target_srv);

			dc->Draw(3, 0);
		}

		{
			// Submit UI draw commands to hatch
			UI_FinalizeDrawBatch();
			
			u32 first_vertex = ht->AddVertices((HT_DrawVertex*)G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count);
			
			int ib_count = G_UI.index_buffer.count;
			for (int i = 0; i < ib_count; i++) {
				G_UI.index_buffer.data[i] += first_vertex;
			}
			
			for (int i = 0; i < UI_STATE.draw_commands.count; i++) {
				UI_DrawCommand* draw_command = &UI_STATE.draw_commands[i];
				ht->AddIndices(&G_UI.index_buffer[draw_command->first_index], draw_command->index_count);
			}
		}
	}
}

static void AssetViewerTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	open_scene = scene;
	open_scene_rect_min = update_info->rect_min;
	open_scene_rect_max = update_info->rect_max;

	SceneEditUpdate(ht, scene);

	for (HT_ItemGroupEach(&scene->entities, i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);

		if (InputIsDown(ht->input_frame, HT_InputKey_Y)) {
			entity->position.x += 0.1f;
		}

		if (InputIsDown(ht->input_frame, HT_InputKey_T)) {
			entity->position.x -= 0.1f;
		}
	}
}

static VertexShader LoadVertexShader(HT_API* ht, HT_StringView shader_path, D3D11_INPUT_ELEMENT_DESC* input_elems, int input_elems_count) {
	ID3DBlob* errors = NULL;

	ID3DBlob* vs_blob = NULL;
	HRESULT vs_res = ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vs_blob, &errors);
	bool ok = vs_res == S_OK;
	if (!ok) {
		char* err = (char*)errors->GetBufferPointer();
		HT_ASSERT(0);
	}

	ID3D11VertexShader* shader;
	ok = ht->D3D11_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);

	ID3D11InputLayout* input_layout = NULL;
	if (input_elems_count > 0) {
		ok = ht->D3D11_device->CreateInputLayout(input_elems, input_elems_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout) == S_OK;
		HT_ASSERT(ok);
	}

	vs_blob->Release();
	return { shader, input_layout };
}

static ID3D11PixelShader* LoadPixelShader(HT_API* ht, HT_StringView shader_path) {
	ID3DBlob* ps_blob;
	bool ok = ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &ps_blob, NULL) == S_OK;
	HT_ASSERT(ok);

	ID3D11PixelShader* shader;
	ok = ht->D3D11_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);
	
	ps_blob->Release();
	return shader;
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

struct MyMessage : Message {
	int x;
	int y;
};

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	FG::ResetTempArena();
	MessageManager::Reset();

	MyMessage send_1;
	send_1.x = 50;
	send_1.y = 40;
	MessageManager::SendNewMessage(send_1);

	MyMessage send_2;
	send_2.x = 400;
	send_2.y = 900;
	MessageManager::SendNewMessage(send_2);

	MyMessage receive_1;
	if (MessageManager::PopNextMessage(&receive_1)) {
		__debugbreak();
	}

	MyMessage receive_2;
	if (MessageManager::PopNextMessage(&receive_2)) {
		__debugbreak();
	}
	__debugbreak();
}

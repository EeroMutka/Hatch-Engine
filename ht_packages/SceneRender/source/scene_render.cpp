#define HT_STATIC_PLUGIN_ID scene_render

#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h

#include <hatch_api.h>

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>

#include <ht_utils/input/input.h>

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>

#include <ht_utils/gizmos/gizmos.h>

#include "../scene_render.inc.ht"

#include <ht_packages/SceneEdit/src/camera.h>
#include <ht_packages/SceneEdit/src/scene_edit.h>

#define TODO() __debugbreak()

// -----------------------------------------------------

struct Globals {
	HT_TabClass* my_tab_class;
};

struct UIBackend {
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

struct Vertex {
	vec3 position;
	vec2 uv;
	vec3 normal;
};

struct RenderMeshPart {
	u32 index_count;
	u32 base_index_location;
	u32 base_vertex_location;
};

struct RenderMesh {
	void* vbo; // ID3D11Buffer
	void* ibo; // ID3D11Buffer
	DS_DynArray(RenderMeshPart) parts;
};

struct RenderTexture {
	void* texture; // ID3D11Texture2D
	void* texture_srv; // ID3D11ShaderResourceView
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* input_layout;
};

struct ShaderConstants {
	mat4 world_to_clip;
};

struct DS_BasicMemSetup {
	DS_Info ds_;
	DS_Arena temp_;
	DS_AllocatorBase hatch_temp_allocator;
	DS_AllocatorBase hatch_heap_allocator;

	// usage helpers
	DS_Arena* temp;
	DS_Info* ds;
	DS_Allocator* heap;
};

// -----------------------------------------------------

static DS_BasicMemSetup G_MEM;

static SceneEditState G_STATE;
static Globals GLOBALS;
static UIBackend G_UI;
static HT_API* G_HT;
static HT_Rect G_RENDER_RECT;
static mat4 G_RENDER_WORLD_TO_CLIP;
static Scene__Scene* G_RENDER_SCENE;

static RenderMesh G_SPRITE_MESH;

static ID3D11Buffer* cbo;
static ID3D11SamplerState* sampler;

static VertexShader main_vs;
static ID3D11PixelShader* main_ps;

static VertexShader present_vs;
static ID3D11PixelShader* present_ps;

static ID3D11InputLayout* input_layout;

static ID3D11Texture2D* color_target;
static ID3D11Texture2D* depth_target;
static ID3D11ShaderResourceView* color_target_srv;
static ID3D11RenderTargetView* color_target_view;
static ID3D11DepthStencilView* depth_target_view;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = G_HT->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = G_HT->AllocatorProc(ptr, size);
	return data;
}

static UI_CachedGlyph UIGetCachedGlyph(u32 codepoint, UI_Font font) {
	HT_CachedGlyph glyph = G_HT->GetCachedGlyph(codepoint);
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

// bind_flags: D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER
static ID3D11Buffer* CreateGPUBuffer(int size, u32 bind_flags, void* data) {
	HT_ASSERT(size > 0);

	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC; // lets do dynamic for simplicity    D3D11_USAGE_DEFAULT;
	desc.BindFlags = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// TODO: We can provide initial data by just passing it as the second arg to CreateBuffer!!!!
	ID3D11Buffer* buffer;
	bool ok = G_HT->D3D11_device->CreateBuffer(&desc, NULL, &buffer) == S_OK;
	HT_ASSERT(ok);

	D3D11_MAPPED_SUBRESOURCE buffer_mapped;
	ok = G_HT->D3D11_device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer_mapped) == S_OK;
	HT_ASSERT(ok && buffer_mapped.pData);

	memcpy(buffer_mapped.pData, data, size);

	G_HT->D3D11_device_context->Unmap(buffer, 0);
	return buffer;
}

static void CreateMesh(RenderMesh* target, Vertex* vertices, int num_vertices, u32* indices, int num_indices) {
	target->vbo = CreateGPUBuffer(num_vertices * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER, vertices);
	target->ibo = CreateGPUBuffer(num_indices * sizeof(u32), D3D11_BIND_INDEX_BUFFER, indices);
}

static void CreateTexture(RenderTexture* target, int width, int height, void* data) {
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* texture_srv;

	D3D11_SUBRESOURCE_DATA texture_initial_data = {};
	texture_initial_data.pSysMem = data;
	texture_initial_data.SysMemPitch = width * 4; // 4 bytes per pixel

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bool ok = G_HT->D3D11_device->CreateTexture2D(&desc, &texture_initial_data, &texture) == S_OK;
	HT_ASSERT(ok);

	ok = G_HT->D3D11_device->CreateShaderResourceView(texture, NULL, &texture_srv) == S_OK;
	HT_ASSERT(ok);

	target->texture = texture;
	target->texture_srv = texture_srv;
}

static VertexShader LoadVertexShader(HT_StringView shader_path, D3D11_INPUT_ELEMENT_DESC* input_elems, int input_elems_count) {
	ID3DBlob* errors = NULL;

	ID3DBlob* vs_blob = NULL;
	HRESULT vs_res = G_HT->D3D11_CompileFromFile(shader_path, NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vs_blob, &errors);
	bool ok = vs_res == S_OK;
	if (!ok) {
		char* err = (char*)errors->GetBufferPointer();
		HT_ASSERT(0);
	}

	ID3D11VertexShader* shader;
	ok = G_HT->D3D11_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);

	ID3D11InputLayout* input_layout = NULL;
	if (input_elems_count > 0) {
		ok = G_HT->D3D11_device->CreateInputLayout(input_elems, input_elems_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout) == S_OK;
		HT_ASSERT(ok);
	}

	vs_blob->Release();
	return { shader, input_layout };
}

static ID3D11PixelShader* LoadPixelShader(HT_StringView shader_path) {
	ID3DBlob* ps_blob;
	bool ok = G_HT->D3D11_CompileFromFile(shader_path, NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &ps_blob, NULL) == S_OK;
	HT_ASSERT(ok);

	ID3D11PixelShader* shader;
	ok = G_HT->D3D11_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);

	ps_blob->Release();
	return shader;
}

static void MaybeRecreateRenderTargets(int width, int height) {
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
		bool ok = G_HT->D3D11_device->CreateTexture2D(&desc, NULL, &color_target) == S_OK;
		HT_ASSERT(ok);

		ok = G_HT->D3D11_device->CreateRenderTargetView(color_target, NULL, &color_target_view) == S_OK;
		HT_ASSERT(ok);

		ok = G_HT->D3D11_device->CreateShaderResourceView(color_target, NULL, &color_target_srv) == S_OK;
		HT_ASSERT(ok);

		D3D11_TEXTURE2D_DESC depth_desc = {};
		depth_desc.Width = width;
		depth_desc.Height = height;
		depth_desc.MipLevels = 1;
		depth_desc.ArraySize = 1;
		depth_desc.SampleDesc.Count = 1;
		depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ok = G_HT->D3D11_device->CreateTexture2D(&depth_desc, NULL, &depth_target) == S_OK;
		HT_ASSERT(ok);

		ok = G_HT->D3D11_device->CreateDepthStencilView(depth_target, NULL, &depth_target_view) == S_OK;
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
	if (G_RENDER_SCENE == NULL) return;

	UI_Init(G_MEM.temp);
	
	vec2 rect_size = G_RENDER_RECT.max - G_RENDER_RECT.min;
	MaybeRecreateRenderTargets((int)rect_size.x, (int)rect_size.y);

	ID3D11DeviceContext* dc = G_HT->D3D11_device_context;

	ShaderConstants constants = {};
	constants.world_to_clip = G_RENDER_WORLD_TO_CLIP;
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
	viewport.Width = rect_size.x;
	viewport.Height = rect_size.y;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	dc->RSSetViewports(1, &viewport);

	// keep from hatch
	// dc->OMSetBlendState(s->blend_state, NULL, 0xffffffff);

	///////////////////////////////////////////////////////////////////////////////////////////

	float clear_color[] = {0.2f, 0.4f, 0.8f, 1.f};
	dc->ClearRenderTargetView(color_target_view, clear_color);
	dc->OMSetRenderTargets(1, &color_target_view, depth_target_view);

	{//if (0) {
		constants.world_to_clip = /*render_object.local_to_world * */G_RENDER_WORLD_TO_CLIP;
		UpdateShaderConstants(dc, constants);
	
		const RenderMesh* mesh_data = &G_SPRITE_MESH;
		//const RenderTexture* color_texture = render_object.color_texture;
	
		for (int i = 0; i < mesh_data->parts.count; i++) {
			RenderMeshPart part = mesh_data->parts[i];
			u32 vbo_offset = 0;
			u32 vbo_stride = sizeof(Vertex);
			dc->IASetVertexBuffers(0, 1, (ID3D11Buffer**)&mesh_data->vbo, &vbo_stride, &vbo_offset);
			dc->IASetIndexBuffer((ID3D11Buffer*)mesh_data->ibo, DXGI_FORMAT_R32_UINT, 0);
	
			//dc->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&color_texture->texture_srv);
			dc->DrawIndexed(part.index_count, part.base_index_location, part.base_vertex_location);
		}
	}

	{
		D3D11_VIEWPORT present_viewport = {};
		present_viewport.TopLeftX = G_RENDER_RECT.min.x;
		present_viewport.TopLeftY = G_RENDER_RECT.min.y;
		present_viewport.Width = rect_size.x;
		present_viewport.Height = rect_size.y;
		present_viewport.MinDepth = 0.f;
		present_viewport.MaxDepth = 1.f;
		dc->RSSetViewports(1, &present_viewport);

		ID3D11RenderTargetView* present_target = G_HT->D3D11_GetHatchRenderTargetView();

		dc->VSSetShader(present_vs.shader, NULL, 0);
		dc->PSSetShader(present_ps, NULL, 0);
		dc->OMSetRenderTargets(1, &present_target, NULL);

		dc->PSSetShaderResources(0, 1, &color_target_srv);

		dc->Draw(3, 0);
	}

	{
		// init UI
		{
			DS_ArrInit(&G_UI.vertex_buffer, G_MEM.temp);
			DS_ArrInit(&G_UI.index_buffer, G_MEM.temp);
			UI_STATE.backend.ResizeAndMapVertexBuffer = UIResizeAndMapVertexBuffer;
			UI_STATE.backend.ResizeAndMapIndexBuffer  = UIResizeAndMapIndexBuffer;
			UI_STATE.backend.GetCachedGlyph = UIGetCachedGlyph;
			
			UI_Inputs ui_inputs = {};
			UI_BeginFrame(&ui_inputs, {0, 18}, {0, 18});
		}

		SceneEditDrawGizmos(ht, &G_STATE, G_RENDER_SCENE);

		UI_FinalizeDrawBatch();
	
		u32 first_vertex = ht->AddVertices((HT_DrawVertex*)G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count);
		for (int i = 0; i < G_UI.index_buffer.count; i++) {
			G_UI.index_buffer.data[i] += first_vertex;
		}
	
		for (int i = 0; i < UI_STATE.draw_commands.count; i++) {
			UI_DrawCommand* draw_command = &UI_STATE.draw_commands.data[i];
			ht->AddIndices(&G_UI.index_buffer[draw_command->first_index], draw_command->index_count);
		}
	}
}

static void DebugSceneTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	HT_ASSERT(scene);

	G_RENDER_RECT = update_info->rect;
	G_RENDER_SCENE = scene;
	G_RENDER_WORLD_TO_CLIP = {};

	SceneEditUpdate(ht, &G_STATE, update_info->rect, ht->input_frame->mouse_position, scene);

	// -----------------------------------------------

	vec2 rect_size = G_RENDER_RECT.max - G_RENDER_RECT.min;

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
			G_RENDER_WORLD_TO_CLIP = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, rect_size.x/rect_size.y, 0.1f, 1000.f);
			break;
		}
	}
}

static void DS_InitBasicMemSetup(DS_BasicMemSetup* setup) {
	setup->ds_ = { &setup->temp_ };
	setup->hatch_temp_allocator = { &setup->ds_, TempAllocatorProc };
	DS_ArenaInit(&setup->temp_, 4096, (DS_Allocator*)&setup->hatch_temp_allocator);	
	setup->hatch_heap_allocator = { &setup->ds_, HeapAllocatorProc };
	setup->temp = &setup->temp_;
	setup->ds = &setup->ds_;
	setup->heap = (DS_Allocator*)&setup->hatch_heap_allocator;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	G_HT = ht;
	DS_InitBasicMemSetup(&G_MEM);

	// create cbo
	D3D11_BUFFER_DESC cbo_desc = {};
	cbo_desc.ByteWidth      = sizeof(ShaderConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	cbo_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cbo_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbo_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	G_HT->D3D11_device->CreateBuffer(&cbo_desc, nullptr, &cbo);

	// create sampler
	D3D11_SAMPLER_DESC sampler_desc = {};
	sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	G_HT->D3D11_device->CreateSamplerState(&sampler_desc, &sampler);

	HT_StringView main_shader_path = HATCH_DIR "/ht_packages/SceneRender/Shaders/main_shader.hlsl";
	HT_StringView present_shader_path = HATCH_DIR "/ht_packages/SceneRender/Shaders/present_shader.hlsl";

	D3D11_INPUT_ELEMENT_DESC main_vs_inputs[] = {
		{    "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{     "UV", 0,    DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	main_vs = LoadVertexShader(main_shader_path, main_vs_inputs, _countof(main_vs_inputs));
	main_ps = LoadPixelShader(main_shader_path);

	present_vs = LoadVertexShader(present_shader_path, NULL, 0);
	present_ps = LoadPixelShader(present_shader_path);

	Vertex sprite_vertices[] = {
		{{-1, -1, 0}, {0, 0}, {0, 0, 1}},
		{{+1, -1, 0}, {0, 0}, {0, 0, 1}},
		{{+1, 1, 0}, {0, 0}, {0, 0, 1}},
		{{-1, 1, 0}, {0, 0}, {0, 0, 1}},
	};
	u32 sprite_indices[] = {0, 1, 2, 0, 2, 3};
	CreateMesh(&G_SPRITE_MESH, sprite_vertices, 4, sprite_indices, 6);
	
	// TODO: fix this ; adding things to the parts array later is invalid as the pointer is to stack mem
	DS_ArrInit(&G_SPRITE_MESH.parts, G_MEM.heap);
	DS_ArrPush(&G_SPRITE_MESH.parts, RenderMeshPart{6, 0, 0});

	G_HT->D3D11_SetRenderProc(Render);

	bool ok = G_HT->RegisterAssetViewerForType(G_HT->types->Scene__Scene, DebugSceneTabUpdate);
	HT_ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	DS_ArenaInit(&G_MEM.temp_, 4096, (DS_Allocator*)&G_MEM.hatch_temp_allocator);

	// OR: maybe the scene edit plugin can act more like a library for the renderer plugin.
	// The renderer plugin can then call functions from the scene edit plugin, like "UpdateScene" and "GenerateGizmos".
	// This sounds good, because it lets us use our current simplistic hatch setup. Let's do it.
}

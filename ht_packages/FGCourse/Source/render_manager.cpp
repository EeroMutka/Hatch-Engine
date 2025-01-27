#define HT_NO_STATIC_PLUGIN_EXPORTS

#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h

#include "common.h"

#include "message_manager.h"
#include "render_manager.h"

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>

#include <ht_utils/gizmos/gizmos.h>

#include "ht_packages/SceneEdit/src/camera.h"
#include "ht_packages/SceneEdit/src/scene_edit.h"

#include "../Shaders/shader_constants.h"

#define DEPTH_MAP_RES 2048

struct ShaderConstants {
	mat4 local_to_clip;
	mat4 local_to_world;
	mat4 world_to_dir_shadow; // world to directional light space for shadow map
	
	vec3 view_position;
	int _pad1;
	
	int point_light_count;
	int spot_light_count;
	float use_specular_value; // negative value means use texture
	int _pad3;

	vec3 directional_light_dir;
	int _pad4;
	vec3 directional_light_emission;
	int _pad5;

	vec4 point_lights_position[16];
	vec4 point_lights_emission[16];

	vec4 spot_lights_direction[16];
	vec4 spot_lights_position[16];
	vec4 spot_lights_emission[16];
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* input_layout;
};

struct UIBackend {
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

// ----------------------------------------------

static UIBackend G_UI;

static ID3D11Buffer* cbo;
static ID3D11SamplerState* g_sampler_nearest;
static ID3D11SamplerState* g_sampler_linear;
static ID3D11SamplerState* g_sampler_percentage_closer;

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

static ID3D11Texture2D* g_dir_light_shadow_map;
static ID3D11DepthStencilView* g_dir_light_shadow_map_dsv;
static ID3D11ShaderResourceView* g_dir_light_shadow_map_srv;

// ----------------------------------------------

static UI_DrawVertex* UIResizeAndMapVertexBuffer(int num_vertices) {
	DS_ArrResizeUndef(&G_UI.vertex_buffer, num_vertices);
	return G_UI.vertex_buffer.data;
}

static u32* UIResizeAndMapIndexBuffer(int num_indices) {
	DS_ArrResizeUndef(&G_UI.index_buffer, num_indices);
	return G_UI.index_buffer.data;
}

static UI_CachedGlyph UIGetCachedGlyph(u32 codepoint, UI_Font font) {
	HT_CachedGlyph glyph = FG::ht->GetCachedGlyph(codepoint);
	return *(UI_CachedGlyph*)&glyph;
}

void RenderManager::CreateTexture(RenderTexture* target, RenderTextureFormat format, int width, int height, void* data) {
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* texture_srv;

	int mip_levels = 1;
	for (int w = width < height ? width : height; w > 2; w /= 2) mip_levels++;
	
	int bytes_per_pixel = format == RenderTextureFormat::RGBA8 ? 4 : 1;

	D3D11_SUBRESOURCE_DATA texture_initial_data[16];
	for (int i = 0; i < mip_levels; i++) {
		texture_initial_data[i].pSysMem = data;
		texture_initial_data[i].SysMemPitch = width * bytes_per_pixel;
	}

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = width;
	desc.Height = height;
	desc.MipLevels = mip_levels;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = format == RenderTextureFormat::RGBA8 ? DXGI_FORMAT_R8G8B8A8_UNORM : DXGI_FORMAT_R8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET; // Using GenMipmaps requires BIND_RENDER_TARGET
	desc.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;
	bool ok = FG::ht->D3D11_device->CreateTexture2D(&desc, texture_initial_data, &texture) == S_OK;
	HT_ASSERT(ok);

	ok = FG::ht->D3D11_device->CreateShaderResourceView(texture, NULL, &texture_srv) == S_OK;
	HT_ASSERT(ok);

	FG::ht->D3D11_device_context->GenerateMips(texture_srv);
	
	target->texture = texture;
	target->texture_srv = texture_srv;
}

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
	bool ok = FG::ht->D3D11_device->CreateBuffer(&desc, NULL, &buffer) == S_OK;
	HT_ASSERT(ok);

	D3D11_MAPPED_SUBRESOURCE buffer_mapped;
	ok = FG::ht->D3D11_device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer_mapped) == S_OK;
	HT_ASSERT(ok && buffer_mapped.pData);

	memcpy(buffer_mapped.pData, data, size);

	FG::ht->D3D11_device_context->Unmap(buffer, 0);
	return buffer;
}

void RenderManager::CreateMesh(RenderMesh* target, Vertex* vertices, int num_vertices, u32* indices, int num_indices) {
	target->vbo = CreateGPUBuffer(num_vertices * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER, vertices);
	target->ibo = CreateGPUBuffer(num_indices * sizeof(u32), D3D11_BIND_INDEX_BUFFER, indices);
}

static VertexShader LoadVertexShader(HT_StringView shader_path, D3D11_INPUT_ELEMENT_DESC* input_elems, int input_elems_count) {
	ID3DBlob* errors = NULL;

	ID3DBlob* vs_blob = NULL;
	HRESULT vs_res = FG::ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vs_blob, &errors);
	bool ok = vs_res == S_OK;
	if (!ok) {
		char* err = (char*)errors->GetBufferPointer();
		HT_ASSERT(0);
	}

	ID3D11VertexShader* shader;
	ok = FG::ht->D3D11_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);

	ID3D11InputLayout* input_layout = NULL;
	if (input_elems_count > 0) {
		ok = FG::ht->D3D11_device->CreateInputLayout(input_elems, input_elems_count, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout) == S_OK;
		HT_ASSERT(ok);
	}

	vs_blob->Release();
	return { shader, input_layout };
}

static ID3D11PixelShader* LoadPixelShader(HT_StringView shader_path) {
	ID3DBlob* errors = NULL;

	ID3DBlob* ps_blob;
	bool ok = FG::ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &ps_blob, &errors) == S_OK;
	if (!ok) {
		char* err = (char*)errors->GetBufferPointer();
		HT_ASSERT(0);
	}

	ID3D11PixelShader* shader;
	ok = FG::ht->D3D11_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &shader) == S_OK;
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
		bool ok = FG::ht->D3D11_device->CreateTexture2D(&desc, NULL, &color_target) == S_OK;
		HT_ASSERT(ok);

		ok = FG::ht->D3D11_device->CreateRenderTargetView(color_target, NULL, &color_target_view) == S_OK;
		HT_ASSERT(ok);

		ok = FG::ht->D3D11_device->CreateShaderResourceView(color_target, NULL, &color_target_srv) == S_OK;
		HT_ASSERT(ok);

		D3D11_TEXTURE2D_DESC depth_desc = {};
		depth_desc.Width = width;
		depth_desc.Height = height;
		depth_desc.MipLevels = 1;
		depth_desc.ArraySize = 1;
		depth_desc.SampleDesc.Count = 1;
		depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ok = FG::ht->D3D11_device->CreateTexture2D(&depth_desc, NULL, &depth_target) == S_OK;
		HT_ASSERT(ok);

		ok = FG::ht->D3D11_device->CreateDepthStencilView(depth_target, NULL, &depth_target_view) == S_OK;
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
	RenderParamsMessage render_params;
	if (!MessageManager::PopNextMessage(&render_params)) return;

	// setup UI / gizmos rendering
	{
		DS_ArrInit(&G_UI.vertex_buffer, FG::mem.temp);
		DS_ArrInit(&G_UI.index_buffer, FG::mem.temp);
		UI_STATE.backend.ResizeAndMapVertexBuffer = UIResizeAndMapVertexBuffer;
		UI_STATE.backend.ResizeAndMapIndexBuffer  = UIResizeAndMapIndexBuffer;

		// setup UI text rendering backend
		UI_STATE.backend.GetCachedGlyph = UIGetCachedGlyph;

		UI_TEMP = FG::mem.temp;
		UI_ResetDrawState();
	}

	vec2 rect_size = render_params.rect.max - render_params.rect.min;
	MaybeRecreateRenderTargets((int)rect_size.x, (int)rect_size.y);

	SceneEditDrawGizmos(FG::ht, render_params.scene_edit_state, render_params.scene);

	ID3D11DeviceContext* dc = FG::ht->D3D11_device_context;

	ShaderConstants constants = {};
	constants.view_position = render_params.view_position;
	//constants.world_to_clip = render_params.world_to_clip;
	//UpdateShaderConstants(dc, constants);

	ID3D11SamplerState* samplers[] = { g_sampler_nearest, g_sampler_linear, g_sampler_percentage_closer };
	dc->PSSetSamplers(0, _countof(samplers), samplers);
	
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dc->IASetInputLayout(main_vs.input_layout);
	dc->VSSetConstantBuffers(0, 1, &cbo);
	dc->PSSetConstantBuffers(0, 1, &cbo);

	// keep from hatch
	// dc->OMSetBlendState(s->blend_state, NULL, 0xffffffff);

	///////////////////////////////////////////////////////////////////////////////////////////

	// Add point lights
	{
		constants.point_light_count = 0;

		AddPointLightMessage msg;
		while (MessageManager::PopNextMessage(&msg)) {
			int i = constants.point_light_count;
			constants.point_lights_position[i].xyz = msg.position;
			constants.point_lights_emission[i].xyz = msg.emission;
			constants.point_light_count++;
		}
	}
	
	// Add spot lights
	{
		constants.spot_light_count = 0;

		AddSpotLightMessage msg;
		while (MessageManager::PopNextMessage(&msg)) {
			int i = constants.spot_light_count;
			constants.spot_lights_position[i].xyz = msg.position;
			constants.spot_lights_direction[i].xyz = msg.direction;
			constants.spot_lights_emission[i].xyz = msg.emission;
			constants.spot_light_count++;
		}
	}
	
	// Add directional light
	{
		constants.directional_light_emission = {};
		
		AddDirectionalLightMessage msg;
		while (MessageManager::PopNextMessage(&msg)) {
			// come up with a directional light matrix...
			// so we basically want to transform the unit box to be around the directional light.
			constants.world_to_dir_shadow =
				M_MatRotateZ(msg.rotation.z * -M_DegToRad) *
				M_MatRotateY(msg.rotation.y * -M_DegToRad) *
				M_MatRotateX(msg.rotation.x * -M_DegToRad) *
				M_MatScale({0.01f, 0.01f, -0.01f}) *
				M_MatTranslate({0, 0, 0.5f});

			mat4 dir_shadow_to_world =
				M_MatRotateX(msg.rotation.x * M_DegToRad) *
				M_MatRotateY(msg.rotation.y * M_DegToRad) *
				M_MatRotateZ(msg.rotation.z * M_DegToRad);

			constants.directional_light_dir = dir_shadow_to_world.row[2].xyz * -1.f;
			constants.directional_light_emission = msg.emission;
		}
	}
	
	DS_DynArray(RenderObjectMessage) render_objects = { FG::mem.temp };
	for (RenderObjectMessage obj; MessageManager::PopNextMessage(&obj);) {
		DS_ArrPush(&render_objects, obj);
	}

	ID3D11ShaderResourceView* shader_resources[3] = {};

	// -- shadow pass ----------------------------------------------------------

	{
		D3D11_VIEWPORT viewport = {};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = DEPTH_MAP_RES;
		viewport.Height = DEPTH_MAP_RES;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		dc->RSSetViewports(1, &viewport);

		dc->ClearDepthStencilView(g_dir_light_shadow_map_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);		
		dc->OMSetRenderTargets(0, NULL, g_dir_light_shadow_map_dsv);

		dc->VSSetShader(main_vs.shader, NULL, 0);
		dc->PSSetShader(NULL, NULL, 0);
		for (int obj_i = 0; obj_i < render_objects.count; obj_i++)
		{
			RenderObjectMessage* render_object = &render_objects[obj_i];
			constants.local_to_clip = render_object->local_to_world * constants.world_to_dir_shadow;
			constants.local_to_world = render_object->local_to_world;
			UpdateShaderConstants(dc, constants);

			const RenderMesh* mesh_data = render_object->mesh;

			for (int i = 0; i < mesh_data->parts.count; i++) {
				RenderMeshPart part = mesh_data->parts[i];
				u32 vbo_offset = 0;
				u32 vbo_stride = sizeof(Vertex);
				dc->IASetVertexBuffers(0, 1, (ID3D11Buffer**)&mesh_data->vbo, &vbo_stride, &vbo_offset);
				dc->IASetIndexBuffer((ID3D11Buffer*)mesh_data->ibo, DXGI_FORMAT_R32_UINT, 0);
				dc->DrawIndexed(part.index_count, part.base_index_location, part.base_vertex_location);
			}
		}
	}

	// -- main pass ----------------------------------------------------------
	
	dc->ClearDepthStencilView(depth_target_view, D3D11_CLEAR_DEPTH, 1.0f, 0);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = rect_size.x;
	viewport.Height = rect_size.y;
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;
	dc->RSSetViewports(1, &viewport);

	shader_resources[2] = g_dir_light_shadow_map_srv;

	float clear_color[] = {0.2f, 0.4f, 0.8f, 1.f};
	dc->ClearRenderTargetView(color_target_view, clear_color);
	dc->OMSetRenderTargets(1, &color_target_view, depth_target_view);

	dc->VSSetShader(main_vs.shader, NULL, 0);
	dc->PSSetShader(main_ps, NULL, 0);
	
	for (int obj_i = 0; obj_i < render_objects.count; obj_i++)
	{
		RenderObjectMessage* render_object = &render_objects[obj_i];
		constants.local_to_clip = render_object->local_to_world * render_params.world_to_clip;
		constants.local_to_world = render_object->local_to_world;
		constants.use_specular_value = render_object->specular_texture ? -1.f : render_object->specular_value;
		UpdateShaderConstants(dc, constants);

		const RenderMesh* mesh_data = render_object->mesh;
		const RenderTexture* color_texture = render_object->color_texture;
		const RenderTexture* specular_texture = render_object->specular_texture;

		for (int i = 0; i < mesh_data->parts.count; i++) {
			RenderMeshPart part = mesh_data->parts[i];
			u32 vbo_offset = 0;
			u32 vbo_stride = sizeof(Vertex);
			dc->IASetVertexBuffers(0, 1, (ID3D11Buffer**)&mesh_data->vbo, &vbo_stride, &vbo_offset);
			dc->IASetIndexBuffer((ID3D11Buffer*)mesh_data->ibo, DXGI_FORMAT_R32_UINT, 0);

			shader_resources[0] = (ID3D11ShaderResourceView*)color_texture->texture_srv;
			shader_resources[1] = (ID3D11ShaderResourceView*)(specular_texture ? specular_texture->texture_srv : color_texture->texture_srv);
			
			dc->PSSetShaderResources(0, _countof(shader_resources), shader_resources);
			dc->DrawIndexed(part.index_count, part.base_index_location, part.base_vertex_location);
		}
	}

	// clear bound resources
	memset(shader_resources, 0, sizeof(shader_resources));
	dc->PSSetShaderResources(0, _countof(shader_resources), shader_resources);

	{
		D3D11_VIEWPORT present_viewport = {};
		present_viewport.TopLeftX = render_params.rect.min.x;
		present_viewport.TopLeftY = render_params.rect.min.y;
		present_viewport.Width = rect_size.x;
		present_viewport.Height = rect_size.y;
		present_viewport.MinDepth = 0.f;
		present_viewport.MaxDepth = 1.f;
		dc->RSSetViewports(1, &present_viewport);

		ID3D11RenderTargetView* present_target = FG::ht->D3D11_GetHatchRenderTargetView();

		dc->VSSetShader(present_vs.shader, NULL, 0);
		dc->PSSetShader(present_ps, NULL, 0);
		dc->OMSetRenderTargets(1, &present_target, NULL);

		dc->PSSetShaderResources(0, 1, &color_target_srv);

		dc->Draw(3, 0);
	}

	{
		// Submit UI draw commands to hatch
		UI_FinalizeDrawBatch();

		u32 first_vertex = FG::ht->AddVertices((HT_DrawVertex*)G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count);

		int ib_count = G_UI.index_buffer.count;
		for (int i = 0; i < ib_count; i++) {
			G_UI.index_buffer.data[i] += first_vertex;
		}

		for (int i = 0; i < UI_STATE.draw_commands.count; i++) {
			UI_DrawCommand* draw_command = &UI_STATE.draw_commands[i];
			FG::ht->AddIndices(&G_UI.index_buffer[draw_command->first_index], draw_command->index_count);
		}
	}

}

void RenderManager::Init() {
	// create cbo
	D3D11_BUFFER_DESC cbo_desc = {};
	cbo_desc.ByteWidth      = sizeof(ShaderConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	cbo_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cbo_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbo_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	FG::ht->D3D11_device->CreateBuffer(&cbo_desc, nullptr, &cbo);

	{
		D3D11_SAMPLER_DESC sampler_desc = {};
		sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
		sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.MaxLOD = 1000.f;
		FG::ht->D3D11_device->CreateSamplerState(&sampler_desc, &g_sampler_nearest);
	}
	{
		D3D11_SAMPLER_DESC sampler_desc = {};
		sampler_desc.Filter         = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		sampler_desc.MaxLOD = 1000.f;
		FG::ht->D3D11_device->CreateSamplerState(&sampler_desc, &g_sampler_linear);
	}
	{
		D3D11_SAMPLER_DESC sampler_desc = {};
		sampler_desc.Filter         = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		sampler_desc.AddressU       = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressV       = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.AddressW       = D3D11_TEXTURE_ADDRESS_CLAMP;
		sampler_desc.ComparisonFunc = D3D11_COMPARISON_LESS;
		sampler_desc.MaxLOD = 1000.f;
		FG::ht->D3D11_device->CreateSamplerState(&sampler_desc, &g_sampler_percentage_closer);
	}

	HT_StringView main_shader_path = "../../ht_packages/FGCourse/Shaders/main_shader.hlsl";
	HT_StringView present_shader_path = "../../ht_packages/FGCourse/Shaders/present_shader.hlsl";

	D3D11_INPUT_ELEMENT_DESC main_vs_inputs[] = {
		{    "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{     "UV", 0,    DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	main_vs = LoadVertexShader(main_shader_path, main_vs_inputs, _countof(main_vs_inputs));
	main_ps = LoadPixelShader(main_shader_path);

	present_vs = LoadVertexShader(present_shader_path, NULL, 0);
	present_ps = LoadPixelShader(present_shader_path);
	
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = DEPTH_MAP_RES;
		desc.Height = DEPTH_MAP_RES;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Format = DXGI_FORMAT_R32_TYPELESS;
		desc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		bool ok = FG::ht->D3D11_device->CreateTexture2D(&desc, NULL, &g_dir_light_shadow_map) == S_OK;
		HT_ASSERT(ok);

		D3D11_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		ok = FG::ht->D3D11_device->CreateDepthStencilView(g_dir_light_shadow_map, &dsv_desc, &g_dir_light_shadow_map_dsv) == S_OK;
		HT_ASSERT(ok);
		
		D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R32_FLOAT;
		srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srv_desc.Texture2D.MipLevels = 1;
		ok = FG::ht->D3D11_device->CreateShaderResourceView(g_dir_light_shadow_map, &srv_desc, &g_dir_light_shadow_map_srv) == S_OK;
		HT_ASSERT(ok);
	}

	FG::ht->D3D11_SetRenderProc(Render);
}

#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"

#include "message_manager.h"
#include "render_manager.h"

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>

#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h

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

// ----------------------------------------------

static UIBackend G_UI;

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

void RenderManager::Init() {
	
}

void RenderManager::RenderAllQueued() {

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

	SceneEditDrawGizmos(ht, &scene_edit_state, open_scene);

	ID3D11DeviceContext* dc = ht->D3D11_device_context;

	ShaderConstants constants = {};
	constants.ws_to_cs = open_scene_ws_to_cs;
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

	RenderObjectMessage render_object;
	while (MessageManager::PopNextMessage(&render_object)) {

		constants.ws_to_cs = ls_to_ws * open_scene_ws_to_cs;
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

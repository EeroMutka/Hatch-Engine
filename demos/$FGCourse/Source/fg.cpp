#define HT_STATIC_PLUGIN_ID fg
#include <hatch_api.h>

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {}
HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {}
HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {}

#if 0
#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h


#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>
#include <ht_utils/input/input.h>

#include "../fg.inc.ht"

#include "../../$SceneEdit/src/camera.h"
#include "../../$SceneEdit/src/scene_edit.h"

// for obj loading
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

// -----------------------------------------------------

static ID3D11Buffer* vbo;
static ID3D11Buffer* ibo;
static int ibo_count;
static ID3D11Buffer* cbo;
static ID3D11VertexShader* vertex_shader;
static ID3D11PixelShader* pixel_shader;
static ID3D11InputLayout* input_layout;

static Scene__Scene* open_scene;
static ivec2 open_scene_rect_min;
static ivec2 open_scene_rect_max;

static ID3D11Texture2D* color_target;
static ID3D11Texture2D* depth_target;
static ID3D11RenderTargetView* color_target_view;
static ID3D11DepthStencilView* depth_target_view;

// -----------------------------------------------------

struct ShaderConstants {
	mat4 ws_to_cs;
};

struct Vertex {
	vec3 position;
};

// bind_flags: D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER
static ID3D11Buffer* CreateGPUBuffer(HT_API* ht, int size, u32 bind_flags, void* data) {
	HT_ASSERT(size > 0);
	
	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC; // lets do dynamic for simplicity    D3D11_USAGE_DEFAULT;
	desc.BindFlags = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* buffer;
	bool ok = ht->D3D11_device->CreateBuffer(&desc, NULL, &buffer) == S_OK;
	HT_ASSERT(ok);

	D3D11_MAPPED_SUBRESOURCE buffer_mapped;
	ok = ht->D3D11_device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer_mapped) == S_OK;
	HT_ASSERT(ok && buffer_mapped.pData);
	
	memcpy(buffer_mapped.pData, data, size);
	
	ht->D3D11_device_context->Unmap(buffer, 0);
	return buffer;
}

static void MaybeRecreateRenderTargets(HT_API* ht, int width, int height) {
	D3D11_TEXTURE2D_DESC desc;
	if (color_target) color_target->GetDesc(&desc);
	if (color_target == NULL || desc.Width != width || desc.Height != height) {
		if (color_target) {
			color_target->Release();
			color_target_view->Release();
			depth_target->Release();
			depth_target_view->Release();
		}

		desc = {};
		desc.Width = width;
		desc.Height = height;
		desc.ArraySize = 1;
		desc.SampleDesc.Count = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET;
		bool ok = ht->D3D11_device->CreateTexture2D(&desc, NULL, &color_target) == S_OK;
		HT_ASSERT(ok);

		ok = ht->D3D11_device->CreateRenderTargetView(color_target, NULL, &color_target_view) == S_OK;
		HT_ASSERT(ok);
		
		D3D11_TEXTURE2D_DESC depth_desc = {};
		depth_desc.Width = width;
		depth_desc.Height = height;
		depth_desc.ArraySize = 1;
		depth_desc.SampleDesc.Count = 1;
		depth_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depth_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		ok = ht->D3D11_device->CreateTexture2D(&desc, NULL, &depth_target) == S_OK;
		HT_ASSERT(ok);

		ok = ht->D3D11_device->CreateDepthStencilView(depth_target, NULL, &depth_target_view) == S_OK;
		HT_ASSERT(ok);
	}
}

static void Render(HT_API* ht) {
	if (open_scene) {
		FGCourse__fg_params_type* params = HT_GetPluginData(FGCourse__fg_params_type, ht);

		int width = open_scene_rect_max.x - open_scene_rect_min.x;
		int height = open_scene_rect_max.y - open_scene_rect_min.y;
		MaybeRecreateRenderTargets(ht, width, height);

		mat4 ws_to_cs = {};
		for (int i = 0; i < open_scene->extended_data.count; i++) {
			HT_Any any = ((HT_Any*)open_scene->extended_data.data)[i];
			if (any.type._struct == params->editor_camera_type) {
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
				ws_to_cs = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, 1280.f / 720.f, 0.1f, 100.f);
			}
		}

		ID3D11DeviceContext* dc = ht->D3D11_device_context;

		// ht->LogInfo("RENDERING");

		D3D11_MAPPED_SUBRESOURCE cbo_mapped;
		dc->Map(cbo, 0, D3D11_MAP_WRITE_DISCARD, 0, &cbo_mapped);
		ShaderConstants* constants = (ShaderConstants*)cbo_mapped.pData;
		constants->ws_to_cs = ws_to_cs;
		dc->Unmap(cbo, 0);

		dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		dc->IASetInputLayout(input_layout);

		dc->VSSetShader(vertex_shader, NULL, 0);
		dc->VSSetConstantBuffers(0, 1, &cbo);

		// keep raster state from hatch
		// dc->RSSetState(raster_state);

		dc->PSSetShader(pixel_shader, NULL, 0);

		dc->OMSetRenderTargets(1, &color_target_view, depth_target_view);

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

		u32 vbo_stride = sizeof(Vertex);
		u32 vbo_offset = 0;
		dc->IASetVertexBuffers(0, 1, &vbo, &vbo_stride, &vbo_offset);
		dc->IASetIndexBuffer(ibo, DXGI_FORMAT_R32_UINT, 0);

		// dc->PSSetShaderResources(0, 1, &texture_srv);

		dc->DrawIndexed(ibo_count, 0, 0);
		
		
		//ID3D11RenderTargetView* color_target = ht->D3D11_GetHatchRenderTargetView();
	}
}

static void AssetViewerTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	FGCourse__fg_params_type* params = HT_GetPluginData(FGCourse__fg_params_type, ht);

	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	open_scene = scene;
	open_scene_rect_min = update_info->rect_min;
	open_scene_rect_max = update_info->rect_max;

	SceneEditUpdate(ht, scene, params->editor_camera_type);

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

static ID3D11VertexShader* LoadVertexShader(HT_API* ht, string shader_path, D3D11_INPUT_ELEMENT_DESC* input_elem_desc) {
	ID3DBlob* vs_blob;
	bool ok = ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vs_blob, NULL) == S_OK;
	HT_ASSERT(ok);

	ID3D11VertexShader* shader;
	ok = ht->D3D11_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);

	ok = ht->D3D11_device->CreateInputLayout(input_elem_desc, ARRAYSIZE(input_elem_desc), vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &input_layout) == S_OK;
	HT_ASSERT(ok);

	vs_blob->Release();
	return shader;
}

static ID3D11PixelShader* LoadPixelShader(HT_API* ht, string shader_path) {
	ID3DBlob* ps_blob;
	bool ok = ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &ps_blob, NULL) == S_OK;
	HT_ASSERT(ok);

	ID3D11PixelShader* shader;
	ok = ht->D3D11_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), NULL, &shader) == S_OK;
	HT_ASSERT(ok);
	
	ps_blob->Release();
	return shader;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	FGCourse__fg_params_type* params = HT_GetPluginData(FGCourse__fg_params_type, ht);
	HT_ASSERT(params);

	// obj loading

	// TODO: fix working directory
	// std::ifstream file("C:/dev/Hatch/test_assets/cube.obj");
	std::ifstream file("C:/dev/Hatch/test_assets/monkey.obj");

	std::vector<vec3> verts;
	std::vector<int> indices;

	if (file.is_open()) {
		std::string line_str;
		while (getline(file, line_str)) {
			std::istringstream line(line_str);

			std::string word;
			line >> word;

			if (word == "v") {
				vec3 vert;
				line >> vert.x;
				line >> vert.y;
				line >> vert.z;
				verts.push_back(vert);
			}
			else if (word == "f") {
				std::vector<int> polygon_indices;

				while (true) {
					std::string vert_word_str;
					line >> vert_word_str;
					if (vert_word_str.size() == 0) break;

					std::istringstream vert_word(vert_word_str);
					int vert_index;
					vert_word >> vert_index;
					polygon_indices.push_back(vert_index-1);
				}

				for (int i = 2; i < polygon_indices.size(); i++) {
					int i1 = polygon_indices[0];
					int i2 = polygon_indices[i];
					int i3 = polygon_indices[i-1];
					indices.push_back(i1);
					indices.push_back(i2);
					indices.push_back(i3);
				}
			}
		}

		file.close();
	}

	// create cbo
	D3D11_BUFFER_DESC cbo_desc = {};
	cbo_desc.ByteWidth      = sizeof(ShaderConstants) + 0xf & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	cbo_desc.Usage          = D3D11_USAGE_DYNAMIC;
	cbo_desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	cbo_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ht->D3D11_device->CreateBuffer(&cbo_desc, nullptr, &cbo);

	vbo = CreateGPUBuffer(ht, (int)verts.size() * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER, verts.data());
	ibo = CreateGPUBuffer(ht, (int)indices.size() * sizeof(u32), D3D11_BIND_INDEX_BUFFER, indices.data());
	ibo_count = (int)indices.size();

	// create vs
	string shader_path = "C:/dev/Hatch/demos/$FGCourse/Shaders/test_shader.hlsl";

	vertex_shader = LoadVertexShader(ht, shader_path);
	pixel_shader = LoadPixelShader(ht, shader_path);

	// create input layout

	D3D11_INPUT_ELEMENT_DESC input_elem_desc[] = {
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	ht->D3D11_SetRenderProc(Render);

	//ok = ht->RegisterAssetViewerForType(params->scene_type, AssetViewerTabUpdate);
	//HT_ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	FGCourse__fg_params_type* params = HT_GetPluginData(FGCourse__fg_params_type, ht);
	HT_ASSERT(params);
	
	/*
	int open_assets_count;
	HT_Asset* open_assets = ht->GetAllOpenAssetsOfType(params->scene_type, &open_assets_count);
	
	open_scene = NULL;

	for (int asset_i = 0; asset_i < open_assets_count; asset_i++) {
		HT_Asset scene_asset = open_assets[asset_i];
		Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, scene_asset);
		open_scene = scene;

		for (HT_ItemGroupEach(&scene->entities, i)) {
			Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);
			
			if (InputIsDown(ht->input_frame, HT_InputKey_Y)) {
				entity->position.x += 0.1f;
			}
			
			if (InputIsDown(ht->input_frame, HT_InputKey_T)) {
				entity->position.x -= 0.1f;
			}
		}
	}*/
}

#endif
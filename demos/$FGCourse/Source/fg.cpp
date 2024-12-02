
//HT_EXPORT void HT_LoadPlugin(HT_API* ht) {}
//HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {}
//HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {}

#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h

#define HT_STATIC_PLUGIN_ID fg
#include <hatch_api.h>

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

struct ShaderConstants {
	mat4 ws_to_cs;
};

struct Vertex {
	vec3 position;
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* input_layout;
};

struct Mesh {
	ID3D11Buffer* vbo;
	ID3D11Buffer* ibo;
	int ibo_count;
};

// -----------------------------------------------------

static Mesh mesh_monkey;
static Mesh mesh_cube;

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

// -----------------------------------------------------

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
			color_target_srv->Release();
			depth_target->Release();
			depth_target_view->Release();
		}

		desc = {};
		desc.Width = width;
		desc.Height = height;
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
				ws_to_cs = ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, (float)width / (float)height, 0.1f, 100.f);
			}
		}

		ID3D11DeviceContext* dc = ht->D3D11_device_context;

		ShaderConstants constants = {};
		constants.ws_to_cs = ws_to_cs;
		UpdateShaderConstants(dc, constants);

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

		u32 vbo_stride = sizeof(Vertex);
		
		{
			u32 vbo_offset = 0;
			dc->IASetVertexBuffers(0, 1, &mesh_monkey.vbo, &vbo_stride, &vbo_offset);
			dc->IASetIndexBuffer(mesh_monkey.ibo, DXGI_FORMAT_R32_UINT, 0);
			dc->DrawIndexed(mesh_monkey.ibo_count, 0, 0);
		}
		
		for (HT_ItemGroupEach(&open_scene->entities, i)) {
			Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &open_scene->entities, i);

			// draw entity as cube
			//Scene__BoxComponent
			
			mat4 ls_to_ws =
				M_MatScale(entity->scale) *
				M_MatRotateX(entity->rotation.x * M_DegToRad) *
				M_MatRotateY(entity->rotation.y * M_DegToRad) *
				M_MatRotateZ(entity->rotation.z * M_DegToRad) *
				M_MatTranslate(entity->position);

			constants.ws_to_cs = ls_to_ws * ws_to_cs;
			UpdateShaderConstants(dc, constants);

			{
				u32 vbo_offset = 0;
				dc->IASetVertexBuffers(0, 1, &mesh_cube.vbo, &vbo_stride, &vbo_offset);
				dc->IASetIndexBuffer(mesh_cube.ibo, DXGI_FORMAT_R32_UINT, 0);
				dc->DrawIndexed(mesh_cube.ibo_count, 0, 0);
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

			dc->PSSetSamplers(0, 1, &sampler);
			dc->PSSetShaderResources(0, 1, &color_target_srv);

			dc->Draw(3, 0);
		}
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

static VertexShader LoadVertexShader(HT_API* ht, string shader_path, D3D11_INPUT_ELEMENT_DESC* input_elems, int input_elems_count) {
	ID3DBlob* errors;

	ID3DBlob* vs_blob;
	bool ok = ht->D3D11_CompileFromFile(shader_path, NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vs_blob, &errors) == S_OK;
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

static Mesh LoadOBJ(HT_API* ht, const char* file_cstr) {

	// TODO: fix working directory
	// std::ifstream file("C:/dev/Hatch/test_assets/cube.obj");
	std::ifstream file(file_cstr);

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

	Mesh mesh = {};
	mesh.vbo = CreateGPUBuffer(ht, (int)verts.size() * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER, verts.data());
	mesh.ibo = CreateGPUBuffer(ht, (int)indices.size() * sizeof(u32), D3D11_BIND_INDEX_BUFFER, indices.data());
	mesh.ibo_count = (int)indices.size();
	return mesh;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	FGCourse__fg_params_type* params = HT_GetPluginData(FGCourse__fg_params_type, ht);
	HT_ASSERT(params);

	// obj loading
	mesh_monkey = LoadOBJ(ht, "C:/dev/Hatch/test_assets/monkey.obj");
	mesh_cube = LoadOBJ(ht, "C:/dev/Hatch/test_assets/cube.obj");
	
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

	string main_shader_path = "C:/dev/Hatch/demos/$FGCourse/Shaders/main_shader.hlsl";
	string present_shader_path = "C:/dev/Hatch/demos/$FGCourse/Shaders/present_shader.hlsl";

	D3D11_INPUT_ELEMENT_DESC main_vs_inputs[] = {
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};
	main_vs = LoadVertexShader(ht, main_shader_path, main_vs_inputs, _countof(main_vs_inputs));
	main_ps = LoadPixelShader(ht, main_shader_path);

	present_vs = LoadVertexShader(ht, present_shader_path, NULL, 0);
	present_ps = LoadPixelShader(ht, present_shader_path);

	ht->D3D11_SetRenderProc(Render);

	bool ok = ht->RegisterAssetViewerForType(params->scene_type, AssetViewerTabUpdate);
	HT_ASSERT(ok);
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

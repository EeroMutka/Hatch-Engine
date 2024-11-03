#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

// for D3D12
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#define HT_INCLUDE_D3D12_API
#include <hatch_api.h>
#include <plugin_utils/basic_data/v0_experimental/basic_data.h>
#include <plugin_utils/string/v0_experimental/string.h>

#include "d3d12_helpers.h"

#include <stdio.h>
#include <assert.h>

/*#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/

// for fbx loading
#include "ufbx.h"

#include "pbr_render.inc.ht"

#define TODO() __debugbreak()

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

struct OpenScene {
	ID3D12Resource* vbo;
	ID3D12Resource* ibo;
	u32 num_vertices;
	u32 num_indices;
};

struct Globals {
	HT_AssetHandle open_scene_asset;
	OpenScene open_scene;
	// DS_Arena open_scene_arena;
	
	ID3D12RootSignature* root_signature;
	
	u64 main_pass_shader_last_modtime;
	ID3D12PipelineState* pipeline_state; // NULL when there are shader errors
};

// -----------------------------------------------------

static Globals GLOBALS;
static DS_Arena* TEMP;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void MaybeHotreloadShader(HT_API* ht) {
	pbr_render_params_type* params = HT_GetPluginData(pbr_render_params_type, ht);
	
	bool hotreload = false;
	if (params) {
		u64 shader_modtime = ht->AssetGetModtime(params->main_pass_shader);
		if (shader_modtime != GLOBALS.main_pass_shader_last_modtime) {
			hotreload = true;
			GLOBALS.main_pass_shader_last_modtime = shader_modtime;
		}
	}
	
	if (hotreload) {
		if (GLOBALS.pipeline_state) {
			GLOBALS.pipeline_state->Release();
			GLOBALS.pipeline_state = NULL;
		}
		
		ID3DBlob* vertex_shader;
		ID3DBlob* pixel_shader;

		UINT compile_flags = 0;
		
		string shader_path = ht->AssetGetFilepath(params->main_pass_shader);
		
		ID3DBlob* errors = NULL;
		HRESULT vs_res = ht->D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, &errors);
		bool vs_ok = vs_res == S_OK;
		if (!vs_ok) {
			ht->DebugPrint(errors ? (char*)errors->GetBufferPointer() : "Error reading shader file\n");
			if (errors) errors->Release();
		}

		errors = NULL;
		HRESULT ps_res = ht->D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, &errors);
		bool ps_ok = ps_res == S_OK;
		if (!ps_ok) {
			ht->DebugPrint(errors ? (char*)errors->GetBufferPointer() : "Error reading shader file\n");
			if (errors) errors->Release();
		}
		
		if (vs_ok && ps_ok) {
			D3D12_INPUT_ELEMENT_DESC input_elem_desc[] = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			};
		
			D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

			pso_desc.InputLayout = { input_elem_desc, DS_ArrayCount(input_elem_desc) };
			pso_desc.pRootSignature = GLOBALS.root_signature;
			pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
			pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };

			pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
			pso_desc.RasterizerState.FrontCounterClockwise = false;
			pso_desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
			pso_desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
			pso_desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			pso_desc.RasterizerState.DepthClipEnable = true;
			pso_desc.RasterizerState.MultisampleEnable = false;
			pso_desc.RasterizerState.AntialiasedLineEnable = false;
			pso_desc.RasterizerState.ForcedSampleCount = 0;
			pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			pso_desc.BlendState.AlphaToCoverageEnable = false;
			pso_desc.BlendState.RenderTarget[0].BlendEnable = true;
			pso_desc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
			pso_desc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			pso_desc.DepthStencilState.DepthEnable = false;
			pso_desc.DepthStencilState.StencilEnable = false;
			pso_desc.SampleMask = UINT_MAX;
			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			pso_desc.SampleDesc.Count = 1;

			bool ok = ht->D3D_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&GLOBALS.pipeline_state)) == S_OK;
			if (!ok) ht->DebugPrint("Failed to create the graphics pipeline!\n");
		}
		
		if (vs_ok) vertex_shader->Release();
		if (ps_ok) pixel_shader->Release();
	}
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	pbr_render_params_type* params = HT_GetPluginData(pbr_render_params_type, ht);
	assert(params);

	bool ok = ht->RegisterAssetViewerForType(params->scene_type);
	assert(ok);
	
	// Create empty root signature
	{
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.pParameters = NULL;
		desc.NumParameters = 0;
		desc.pStaticSamplers = NULL;
		desc.NumStaticSamplers = 0;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature = NULL;
		ok = ht->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, NULL) == S_OK;
		assert(ok);

		ok = ht->D3D_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&GLOBALS.root_signature)) == S_OK;
		assert(ok);

		signature->Release();
	}
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	GLOBALS.root_signature->Release();
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	MaybeHotreloadShader(ht);
}

static void LoadScene(HT_API* ht, HT_AssetHandle scene_asset) {
	Scene* scene = HT_GetAssetData(Scene, ht, scene_asset);
	assert(scene);
	
	Mesh* mesh = HT_GetAssetData(Mesh, ht, scene->first_mesh);
	assert(mesh);
	
	string mesh_filepath = ht->AssetGetFilepath(mesh->source_asset);
	const char* mesh_filepath_cstr = STR_ToC(TEMP, mesh_filepath);
	
	ufbx_load_opts opts = {0}; // Optional, pass NULL for defaults
	ufbx_error error; // Optional, pass NULL if you don't care about errors
	ufbx_scene* fbx_scene = ufbx_load_file(mesh_filepath_cstr, &opts, &error);
	if (!fbx_scene) {
		printf("Failed to load: %s\n", error.description.data);
		assert(0);
	}
	
	DS_DynArray(vec3) vertices = {TEMP};
	DS_DynArray(u32) indices = {TEMP};

	// Let's just list all objects within the scene for example:
	for (size_t i = 0; i < fbx_scene->nodes.count; i++) {
		ufbx_node* node = fbx_scene->nodes.data[i];
		if (node->is_root) continue;
		
		// printf("Object: %s\n", node->name.data);
		if (node->mesh) {
			// load mesh!
			ufbx_mesh* fbx_mesh = node->mesh;
			
			u32 triangulated_face[32*3];
			assert(fbx_mesh->max_face_triangles < 32);
			
			for (size_t face_i = 0; face_i < fbx_mesh->faces.count; face_i++) {
				ufbx_face face = fbx_mesh->faces[face_i];
				u32 num_triangles = ufbx_triangulate_face(triangulated_face, 32*3, fbx_mesh, face);
				u32 num_tri_indices = num_triangles*3;
				for (u32 index_i = 0; index_i < num_tri_indices; index_i++) {
					u32 vertex_i = fbx_mesh->vertex_indices[triangulated_face[index_i]];
					DS_ArrPush(&indices, vertex_i);
				}
			}
			
			DS_ArrResizeUndef(&vertices, (int)fbx_mesh->vertices.count);
			for (size_t vert_i = 0; vert_i < fbx_mesh->vertices.count; vert_i++) {
				ufbx_vec3* fbx_vert = &fbx_mesh->vertices[vert_i];
				vertices[vert_i] = {(float)fbx_vert->x, (float)fbx_vert->y, (float)fbx_vert->z};
			}
			
			// printf("-> mesh with %zu faces\n", mesh->faces.count);
		}
	}
	
	ufbx_free_scene(fbx_scene);
	
	//float vertices[] = {
	//	-1.f, -1.f,
	//	1.f, -1.f,
	//	0.f, 1.f,
	//};
	//int indices[] = {0, 1, 2};
	
	TempGPUCmdContext ctx = CreateTempGPUCmdContext(ht);
	
	GLOBALS.open_scene.vbo = CreateGPUBuffer(&ctx, vertices.count * sizeof(vec3), vertices.data);
	GLOBALS.open_scene.num_vertices = vertices.count;
	
	GLOBALS.open_scene.ibo = CreateGPUBuffer(&ctx, indices.count * sizeof(u32), indices.data);
	GLOBALS.open_scene.num_indices = indices.count;
	
	DestroyTempGPUCmdContext(&ctx);
}

HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list) {
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	DS_Arena temp_arena;
	DS_ArenaInit(&temp_arena, 1024, temp_allocator);
	TEMP = &temp_arena;
	
	HT_AssetHandle open_scene_asset = NULL;
	
	HT_AssetViewerTabUpdate updates[8];
	int updates_count = 0;
	
	if (GLOBALS.pipeline_state) {
		HT_AssetViewerTabUpdate update;
		while (ht->PollNextAssetViewerTabUpdate(&update)) {
			open_scene_asset = update.data_asset;
			updates[updates_count++] = update;
			assert(updates_count <= 8);
		}
	}
	
	if (open_scene_asset != GLOBALS.open_scene_asset) {
		if (GLOBALS.open_scene_asset) {
			TODO(); // unload the currently open scene
		}
		
		LoadScene(ht, open_scene_asset);
		GLOBALS.open_scene_asset = open_scene_asset;
	}
	
	for (int i = 0; i < updates_count; i++) {
		HT_AssetViewerTabUpdate update = updates[i];
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = update.rect_min.x;
		viewport.TopLeftY = update.rect_min.y;
		viewport.Width = update.rect_max.x - update.rect_min.x;
		viewport.Height = update.rect_max.y - update.rect_min.y;
		command_list->RSSetViewports(1, &viewport);
	
		command_list->SetPipelineState(GLOBALS.pipeline_state);
		command_list->SetGraphicsRootSignature(GLOBALS.root_signature);
	
		D3D12_VERTEX_BUFFER_VIEW vbo_view;
		vbo_view.BufferLocation = GLOBALS.open_scene.vbo->GetGPUVirtualAddress();
		vbo_view.StrideInBytes = 3 * sizeof(float); // position
		vbo_view.SizeInBytes = GLOBALS.open_scene.num_vertices * vbo_view.StrideInBytes;
		command_list->IASetVertexBuffers(0, 1, &vbo_view);

		D3D12_INDEX_BUFFER_VIEW ibo_view;
		ibo_view.BufferLocation = GLOBALS.open_scene.ibo->GetGPUVirtualAddress();
		ibo_view.SizeInBytes = GLOBALS.open_scene.num_indices * sizeof(u32);
		ibo_view.Format = DXGI_FORMAT_R32_UINT;
		command_list->IASetIndexBuffer(&ibo_view);
	
		command_list->DrawIndexedInstanced(GLOBALS.open_scene.num_indices, 1, 0, 0, 0);
	}
	
	TEMP = NULL;
}
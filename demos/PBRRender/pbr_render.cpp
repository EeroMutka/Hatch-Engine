// This file is part of the Hatch codebase, written by Eero Mutka.
// For educational purposes only.

#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

// for D3D12
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#define HT_INCLUDE_D3D12_API
#include <hatch_api.h>
#include <plugin_utils/basic_data/v1/basic_data.h>
#include <plugin_utils/string/v1/string.h>
#include <plugin_utils/math/v1/HandmadeMath.h>
#include <plugin_utils/input/v1/input.h>

#include "d3d12_helpers.h"
#include "camera.h"

#include <stdio.h>
#include <assert.h>

/*#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"*/

// for fbx loading
#include "ufbx.h"

#include "PBRRender.inc.ht"

#define TODO() __debugbreak()

static const vec4 CLEAR_COLOR = {0.3f, 0.4f, 0.9f, 1.0f};

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

struct LoadedMeshPart {
	u32 index_count;
	u32 base_index_location;
	u32 base_vertex_location;
};

struct LoadedScene {
	bool is_loaded;
	DS_DynArray(LoadedMeshPart) mesh_parts;
	ID3D12Resource* vbo;
	ID3D12Resource* ibo;
	u32 vbo_size;
	u32 ibo_size;
};

struct Vertex {
	vec3 position;
	vec3 normal;
};

struct Shader {
	u64 last_modtime;
	ID3D12PipelineState* pipeline; // NULL if there's compile errors
};

struct Globals {
	HT_AssetHandle current_scene_asset;
	LoadedScene current_scene;
	
	ivec2 render_targets_size;
	ID3D12Resource* color_rt;
	ID3D12Resource* depth_rt;
	
	Camera camera;
	
	ID3D12DescriptorHeap* rtv_heap; // render target view heap
	ID3D12DescriptorHeap* srv_heap; // shader resource view heap
	ID3D12DescriptorHeap* dsv_heap; // depth-stencil view descriptor heap
	ID3D12RootSignature* root_signature;
	
	Shader main_pass_shader;
	Shader present_shader;
	
	bool all_shaders_ok;
};

// -----------------------------------------------------

static Globals GLOBALS;
static DS_Allocator* HEAP;
static DS_Arena* TEMP;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static void* HeapAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->AllocatorProc(ptr, size);
	return data;
}

struct ShaderParams {
	bool enable_depth_test;
};

static void UnloadShader(Shader* shader) {
	if (shader->pipeline) {
		shader->pipeline->Release();
	}
	shader->pipeline = NULL;
}

static bool MaybeReloadShader(HT_API* ht, Shader* shader, HT_AssetHandle shader_asset, const ShaderParams* params) {
	bool reload = false;
	
	u64 modtime = ht->AssetGetModtime(shader_asset);
	assert(modtime != 0);

	if (modtime != shader->last_modtime) {
		reload = true;
		shader->last_modtime = modtime;
	}
	
	if (reload) {
		UnloadShader(shader);
		
		ID3DBlob* vertex_shader;
		ID3DBlob* pixel_shader;

		UINT compile_flags = 0;
		
		string shader_path = ht->AssetGetFilepath(shader_asset);
		
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
				{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
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
			
			pso_desc.DepthStencilState.DepthEnable = params->enable_depth_test;
			pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			D3D12_DEPTH_STENCILOP_DESC stencil_op = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
			pso_desc.DepthStencilState.FrontFace = stencil_op;
			pso_desc.DepthStencilState.BackFace = stencil_op;
			
			pso_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;

			pso_desc.BlendState.AlphaToCoverageEnable = false;
			pso_desc.BlendState.RenderTarget[0].BlendEnable = true;
			pso_desc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
			pso_desc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
			pso_desc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
			pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			pso_desc.SampleMask = UINT_MAX;
			pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			pso_desc.NumRenderTargets = 1;
			pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			pso_desc.SampleDesc.Count = 1;

			bool ok = ht->D3D_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&shader->pipeline)) == S_OK;
			if (!ok) ht->DebugPrint("Failed to create the graphics pipeline!\n");
		}
		
		if (vs_ok) vertex_shader->Release();
		if (ps_ok) pixel_shader->Release();
	}
	
	return shader->pipeline != NULL;
}

static void DestroyRenderTargetsIfPossible() {
	if (GLOBALS.render_targets_size.x > 0) {
		GLOBALS.color_rt->Release();
		GLOBALS.depth_rt->Release();
	}
	GLOBALS.render_targets_size = {};
}

static void MaybeResizeRenderTargets(ID3D12Device* device, ivec2 new_size) {
	if (new_size.x == GLOBALS.render_targets_size.x && new_size.y == GLOBALS.render_targets_size.y) return;
	DestroyRenderTargetsIfPossible();
	
	GLOBALS.render_targets_size = new_size;
	
	// Create color rendertarget
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = (u64)new_size.x;
		desc.Height = (u64)new_size.y;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		
		D3D12_HEAP_PROPERTIES props = {D3D12_HEAP_TYPE_DEFAULT};
		
		D3D12_CLEAR_VALUE optimized_clear = {};
		optimized_clear.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		memcpy(&optimized_clear.Color, &CLEAR_COLOR.x, sizeof(vec4));

		bool ok = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_RENDER_TARGET, &optimized_clear, IID_PPV_ARGS(&GLOBALS.color_rt)) == S_OK;
		assert(ok);
		
		device->CreateRenderTargetView(GLOBALS.color_rt, NULL, GLOBALS.rtv_heap->GetCPUDescriptorHandleForHeapStart());
	}
		
	// Create depth buffer
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = (u64)new_size.x;
		desc.Height = (u64)new_size.y;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_D32_FLOAT;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		
		D3D12_HEAP_PROPERTIES props = {D3D12_HEAP_TYPE_DEFAULT};
		
		D3D12_CLEAR_VALUE depth_optimized_clear = {};
		depth_optimized_clear.Format = DXGI_FORMAT_D32_FLOAT;
		depth_optimized_clear.DepthStencil.Depth = 1.0f;
		depth_optimized_clear.DepthStencil.Stencil = 0;

		bool ok = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_DEPTH_WRITE, &depth_optimized_clear, IID_PPV_ARGS(&GLOBALS.depth_rt)) == S_OK;
		assert(ok);
		
		/*D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
		dsv_desc.Format = DXGI_FORMAT_D32_FLOAT;
		dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
		device->CreateDepthStencilView(GLOBALS.depth_rt, &dsv_desc, GLOBALS.dsv_heap->GetCPUDescriptorHandleForHeapStart());*/
		device->CreateDepthStencilView(GLOBALS.depth_rt, NULL, GLOBALS.dsv_heap->GetCPUDescriptorHandleForHeapStart());
	}
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	PBRRenderParams* params = HT_GetPluginData(PBRRenderParams, ht);
	assert(params);

	bool ok = ht->RegisterAssetViewerForType(params->scene_type);
	assert(ok);
	
	// Create root signature
	{
		D3D12_DESCRIPTOR_RANGE descriptor_range = {};
		descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptor_range.NumDescriptors = 1;
		descriptor_range.BaseShaderRegister = 0;
		descriptor_range.RegisterSpace = 0;
		descriptor_range.OffsetInDescriptorsFromTableStart = 0;
		
		D3D12_ROOT_PARAMETER root_params[2] = {};
		root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		root_params[0].Constants.ShaderRegister = 0;
		root_params[0].Constants.RegisterSpace = 0;
		root_params[0].Constants.Num32BitValues = 16;
		root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_params[1].DescriptorTable.NumDescriptorRanges = 1;
		root_params[1].DescriptorTable.pDescriptorRanges = &descriptor_range;
		root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.pParameters = root_params;
		desc.NumParameters = DS_ArrayCount(root_params);
		desc.pStaticSamplers = &sampler;
		desc.NumStaticSamplers = 1;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature = NULL;
		ok = ht->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, NULL) == S_OK;
		assert(ok);

		ok = ht->D3D_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&GLOBALS.root_signature)) == S_OK;
		assert(ok);

		signature->Release();
	}
	
	// Create a render target view descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
		rtv_heap_desc.NumDescriptors = 1;
		rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		ok = ht->D3D_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&GLOBALS.rtv_heap)) == S_OK;
		assert(ok);
	}
	
	// Create a shader resource view heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
		srv_heap_desc.NumDescriptors = 1;
		srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ok = ht->D3D_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&GLOBALS.srv_heap)) == S_OK;
		assert(ok);
	}
		
	// Create a depth stencil view descriptor heap
	{
		D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc = {};
		dsv_heap_desc.NumDescriptors = 1;
		dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ok = ht->D3D_device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(&GLOBALS.dsv_heap)) == S_OK;
		assert(ok);
	}
	
	GLOBALS.dsv_heap->SetName(L"Depth/Stencil Heap");
	
	GLOBALS.camera.pos = {0.f, -5.f, 0.f};
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	PBRRenderParams* params = HT_GetPluginData(PBRRenderParams, ht);
	
	ShaderParams main_pass_params{};
	main_pass_params.enable_depth_test = true;
	bool ok = MaybeReloadShader(ht, &GLOBALS.main_pass_shader, params->main_pass_shader, &main_pass_params);
	
	ShaderParams present_params{};
	present_params.enable_depth_test = false;
	ok = ok && MaybeReloadShader(ht, &GLOBALS.present_shader, params->present_shader, &present_params);
	
	GLOBALS.all_shaders_ok = ok;
	
	/*if (InputIsDown(ht->input_frame, HT_InputKey_P)) {
		printf("Pressing P\n");
	}*/
}

static void UnloadSceneIfPossible(HT_API* ht, LoadedScene* scene) {
	if (scene->is_loaded) {
		DS_ArrDeinit(&scene->mesh_parts);
		scene->ibo->Release();
		scene->vbo->Release();
		*scene = {};
	}
}

static void LoadScene(HT_API* ht, LoadedScene* scene, HT_AssetHandle scene_asset) {
	Scene* scene_info = HT_GetAssetData(Scene, ht, scene_asset);
	assert(scene_info);
	
	Mesh* mesh = HT_GetAssetData(Mesh, ht, scene_info->first_mesh);
	assert(mesh);
	
	string mesh_filepath = ht->AssetGetFilepath(mesh->source_asset);
	const char* mesh_filepath_cstr = STR_ToC(TEMP, mesh_filepath);
	
	ufbx_load_opts opts = {0}; // Optional, pass NULL for defaults
	opts.generate_missing_normals = true;
	
	ufbx_error error; // Optional, pass NULL if you don't care about errors
	ufbx_scene* fbx_scene = ufbx_load_file(mesh_filepath_cstr, &opts, &error);
	if (!fbx_scene) {
		printf("Failed to load: %s\n", error.description.data);
		assert(0);
	}
	
	DS_ArrInit(&scene->mesh_parts, HEAP);
	
	struct TempPart {
		Vertex* vertices;
		u32 base_vertex;
		u32 num_vertices;
		u32* indices;
		u32 base_index;
		u32 num_indices;
	};
	
	DS_DynArray(TempPart) temp_parts = {TEMP};
	u32 total_num_vertices = 0;
	u32 total_num_indices = 0;
	
	for (size_t node_i = 0; node_i < fbx_scene->nodes.count; node_i++) {
		ufbx_node* node = fbx_scene->nodes.data[node_i];
		if (node->is_root) continue;
		
		if (node->mesh) {
			// see docs at https://ufbx.github.io/elements/meshes/
			ufbx_mesh* fbx_mesh = node->mesh;
			
			u32 num_tri_indices = (u32)fbx_mesh->max_face_triangles * 3;
			u32* tri_indices = (u32*)DS_ArenaPush(TEMP, num_tri_indices * sizeof(u32));
			
			for (u32 part_i = 0; part_i < fbx_mesh->material_parts.count; part_i++) {
				ufbx_mesh_part* part = &fbx_mesh->material_parts[part_i];
				
				u32 num_triangles = (u32)part->num_triangles;
				Vertex* vertices = (Vertex*)DS_ArenaPush(TEMP, num_triangles * 3 * sizeof(Vertex));
				u32 num_vertices = 0;
				
				for (u32 face_i = 0; face_i < part->num_faces; face_i++) {
					ufbx_face face = fbx_mesh->faces.data[part->face_indices.data[face_i]];
					
					u32 num_triangulated_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, fbx_mesh, face);
					
					for (u32 i = 0; i < num_triangulated_tris*3; i++) {
						u32 index = tri_indices[i];
						
						Vertex* v = &vertices[num_vertices++];
						ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
						ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
						v->position = {(float)position.x, (float)position.y, (float)position.z};
						v->normal = {(float)normal.x, (float)normal.y, (float)normal.z};
						// v->uv = ufbx_get_vertex_vec3(&fbx_mesh->vertex_uv, index);
					}
				}
				
				assert(num_vertices == num_triangles * 3);
				
				// Generate index buffer
				ufbx_vertex_stream streams[1] = {
					{ vertices, num_vertices, sizeof(Vertex) },
				};
				u32 num_indices = num_triangles * 3;
				u32* indices = (u32*)DS_ArenaPush(TEMP, num_indices * sizeof(u32));
				
				// This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
				// indices are written in `indices[]` and the number of unique vertices is returned.
				num_vertices = (u32)ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);
				
				TempPart temp_part = {
					vertices, total_num_vertices, num_vertices,
					indices, total_num_indices, num_indices,
				};
				DS_ArrPush(&temp_parts, temp_part);
				
				LoadedMeshPart loaded_part = {};
				loaded_part.base_index_location = total_num_indices;
				loaded_part.base_vertex_location = total_num_vertices;
				loaded_part.index_count = num_indices;
				DS_ArrPush(&scene->mesh_parts, loaded_part);
				
				total_num_indices += num_indices;
				total_num_vertices += num_vertices;
			}
		}
	}
	
	ufbx_free_scene(fbx_scene);
	
	TempGPUCmdContext ctx = CreateTempGPUCmdContext(ht);
	
	// Allocating a temporary "combined" array is unnecessary and required by the `initial_data` API for CreateGPUBuffer.
	// If we instead mapped the buffer pointer, we could copy directly to it. But this is good enough for now.
	Vertex* combined_vertices = (Vertex*)DS_ArenaPush(TEMP, total_num_vertices * sizeof(Vertex));
	u32* combined_indices = (u32*)DS_ArenaPush(TEMP, total_num_indices * sizeof(u32));
	
	for (int i = 0; i < temp_parts.count; i++) {
		TempPart temp_part = temp_parts[i];
		memcpy((Vertex*)combined_vertices + temp_part.base_vertex, temp_part.vertices, temp_part.num_vertices * sizeof(Vertex));
		memcpy((u32*)combined_indices + temp_part.base_index, temp_part.indices, temp_part.num_indices * sizeof(u32));
	}
	
	scene->vbo = CreateGPUBuffer(&ctx, total_num_vertices * sizeof(Vertex), combined_vertices);
	scene->vbo_size = total_num_vertices * sizeof(Vertex);
	
	scene->ibo = CreateGPUBuffer(&ctx, total_num_indices * sizeof(u32), combined_indices);
	scene->ibo_size = total_num_indices * sizeof(u32);
	
	scene->is_loaded = true;
	
	DestroyTempGPUCmdContext(&ctx);
}

HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list) {
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	DS_Arena temp_arena;
	DS_ArenaInit(&temp_arena, 1024, temp_allocator);
	TEMP = &temp_arena;
	
	Allocator _heap_allocator = {{HeapAllocatorProc}, ht};
	HEAP = (DS_Allocator*)&_heap_allocator;
	
	HT_AssetHandle current_scene_asset = NULL;
	
	HT_AssetViewerTabUpdate updates[8];
	int updates_count = 0;
	
	if (GLOBALS.all_shaders_ok) {
		HT_AssetViewerTabUpdate update;
		while (ht->PollNextAssetViewerTabUpdate(&update)) {
			current_scene_asset = update.data_asset;
			updates[updates_count++] = update;
			assert(updates_count <= 8);
		}
	}
	
	if (current_scene_asset != GLOBALS.current_scene_asset) {
		UnloadSceneIfPossible(ht, &GLOBALS.current_scene);
		if (current_scene_asset) {
			LoadScene(ht, &GLOBALS.current_scene, current_scene_asset);
		}
		GLOBALS.current_scene_asset = current_scene_asset;
	}
	
	for (int update_i = 0; update_i < updates_count; update_i++) {
		HT_AssetViewerTabUpdate update = updates[update_i];
		ivec2 viewport_size = {update.rect_max.x - update.rect_min.x, update.rect_max.y - update.rect_min.y};
		MaybeResizeRenderTargets(ht->D3D_device, viewport_size);
		
		command_list->SetGraphicsRootSignature(GLOBALS.root_signature);
		command_list->SetDescriptorHeaps(1, &GLOBALS.srv_heap);

		// --- draw offscreen ---
		
		float aspect_ratio = (float)viewport_size.x / (float)viewport_size.y;
		UpdateCamera(&GLOBALS.camera, ht->input_frame, 0.01f, 0.001f, 70.f, aspect_ratio, 0.01f, 1000.f);
		
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = GLOBALS.rtv_heap->GetCPUDescriptorHandleForHeapStart();
		D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = GLOBALS.dsv_heap->GetCPUDescriptorHandleForHeapStart();
		command_list->OMSetRenderTargets(1, &rtv_handle, false, &dsv_handle);
		
		command_list->ClearRenderTargetView(rtv_handle, CLEAR_COLOR._, 0, NULL);
		command_list->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
		
		D3D12_VIEWPORT offscreen_viewport = {};
		offscreen_viewport.Width = (float)viewport_size.x;
		offscreen_viewport.Height = (float)viewport_size.y;
		offscreen_viewport.MinDepth = 0.f;
		offscreen_viewport.MaxDepth = 1.f;
		command_list->RSSetViewports(1, &offscreen_viewport);
		
		command_list->SetPipelineState(GLOBALS.main_pass_shader.pipeline);
		command_list->SetGraphicsRoot32BitConstants(0, 16, &GLOBALS.camera.clip_from_world, 0);
	
		D3D12_VERTEX_BUFFER_VIEW vbo_view;
		vbo_view.BufferLocation = GLOBALS.current_scene.vbo->GetGPUVirtualAddress();
		vbo_view.StrideInBytes = sizeof(Vertex);
		vbo_view.SizeInBytes = GLOBALS.current_scene.vbo_size;
		command_list->IASetVertexBuffers(0, 1, &vbo_view);
		
		D3D12_INDEX_BUFFER_VIEW ibo_view;
		ibo_view.BufferLocation = GLOBALS.current_scene.ibo->GetGPUVirtualAddress();
		ibo_view.SizeInBytes = GLOBALS.current_scene.ibo_size;
		ibo_view.Format = DXGI_FORMAT_R32_UINT;
		command_list->IASetIndexBuffer(&ibo_view);
	
		for (int i = 0; i < GLOBALS.current_scene.mesh_parts.count; i++) {
			LoadedMeshPart part = GLOBALS.current_scene.mesh_parts[i];
			command_list->DrawIndexedInstanced(part.index_count, 1, part.base_index_location, part.base_vertex_location, 0);
		}
		
		// ---------------------
		
		D3D12_RESOURCE_BARRIER color_rt_to_shader_read = GPUTransition(GLOBALS.color_rt, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		command_list->ResourceBarrier(1, &color_rt_to_shader_read);
		
		// --- draw to hatch ---
		
		// update shader resources
		ht->D3D_device->CreateShaderResourceView(GLOBALS.color_rt, NULL, GLOBALS.srv_heap->GetCPUDescriptorHandleForHeapStart());
		
		D3D12_GPU_DESCRIPTOR_HANDLE descriptor_table = GLOBALS.srv_heap->GetGPUDescriptorHandleForHeapStart();
		command_list->SetGraphicsRootDescriptorTable(1, descriptor_table);
		
		D3D12_VIEWPORT present_viewport = {};
		present_viewport.TopLeftX = (float)update.rect_min.x;
		present_viewport.TopLeftY = (float)update.rect_min.y;
		present_viewport.Width = (float)viewport_size.x;
		present_viewport.Height = (float)viewport_size.y;
		command_list->RSSetViewports(1, &present_viewport);
		
		D3D12_CPU_DESCRIPTOR_HANDLE present_rtv_handle = ht->D3DGetHatchRenderTargetView();
		command_list->OMSetRenderTargets(1, &present_rtv_handle, false, NULL);
		
		command_list->SetPipelineState(GLOBALS.present_shader.pipeline);
		
		command_list->DrawInstanced(3, 1, 0, 0);
		
		// ---------------------

		D3D12_RESOURCE_BARRIER shader_read_to_color_rt = GPUTransition(GLOBALS.color_rt, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		command_list->ResourceBarrier(1, &shader_read_to_color_rt);
	}
	
	TEMP = NULL;
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	DestroyRenderTargetsIfPossible();
	UnloadSceneIfPossible(ht, &GLOBALS.current_scene);
	UnloadShader(&GLOBALS.main_pass_shader);
	UnloadShader(&GLOBALS.present_shader);
	GLOBALS.root_signature->Release();
}
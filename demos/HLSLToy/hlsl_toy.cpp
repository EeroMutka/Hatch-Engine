#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

// For D3D12
#define HT_INCLUDE_D3D12_API
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "hlsl_toy.inc.ht"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

#include <assert.h>

#define TODO() __debugbreak()


struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

struct Globals {
	ID3D12RootSignature* root_signature;
	ID3D12PipelineState* pipeline_state;
};

// -----------------------------------------------------

static Globals GLOBALS;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	// Compile shaders
	

	// So we want to load this plugin data once and make root signature once...
	// Well we could just use a global I suppose.
	
	// Create empty root signature
	{
		
		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.pParameters = NULL;
		desc.NumParameters = 0;
		desc.pStaticSamplers = NULL;
		desc.NumStaticSamplers = 0;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature = NULL;
		bool ok = ht->D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, NULL) == S_OK;
		assert(ok);

		ok = ht->D3D_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&GLOBALS.root_signature)) == S_OK;
		assert(ok);

		signature->Release();
	}
	
	// Create the pipeline
	{
		ID3DBlob* vertex_shader;
		ID3DBlob* pixel_shader;

		// #ifdef UI_DX12_DEBUG_MODE
		//		UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		// #else
		UINT compile_flags = 0;
		// #endif

		// TODO: AssetGetFilepath
		const wchar_t* shader_path = L"C:/dev/Hatch/demos/HLSLToy/test_shader.hlsl";
		
		ID3DBlob* errors;
		bool ok = ht->D3DCompileFromFile(shader_path, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, &errors) == S_OK;
		if (!ok) {
			const char* err = (const char*)errors->GetBufferPointer();
			TODO();
		}

		ok = ht->D3DCompileFromFile(shader_path, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, &errors) == S_OK;
		if (!ok) {
			const char* err = (const char*)errors->GetBufferPointer();
			TODO();
		}
		
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

		pso_desc.InputLayout = { NULL, 0 };
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

		ok = ht->D3D_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&GLOBALS.pipeline_state)) == S_OK;
		assert(ok);
		
		vertex_shader->Release();
		pixel_shader->Release();
	}
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	GLOBALS.pipeline_state->Release();
	GLOBALS.root_signature->Release();
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	
	// TODO: type safety
	params_type* params = HT_GetPluginData(params_type, ht);
	if (params) {
		params->testing += 0.02f;
		
		// string path = ht->AssetGetFilepath(params->shader_file);
	}
	
}

HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list) {
	// build draw commands.
	
	// rendertarget is already bound
	// primitivetopology is already set
	
	/*D3D12_VIEWPORT viewport = {};
	viewport.TopLeftX = 200.f;
	viewport.TopLeftY = 200.f;
	viewport.Width = 200.f;
	viewport.Height = 200.f;
	command_list->RSSetViewports(1, &viewport);*/
	
	command_list->SetPipelineState(GLOBALS.pipeline_state);
	command_list->SetGraphicsRootSignature(GLOBALS.root_signature);
	
	command_list->DrawInstanced(3, 1, 0, 0);
}
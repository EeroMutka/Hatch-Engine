// The goal of the texture viewer plugin:
// - Make a new tab class that you can use to preview textures.
// - So here's how it should work: Hatch has no concept of "currently opened things" - the currently opened state should be stored per tab, by the plugin. But we could implement "double clicking" an asset as an API that the plugin could define. Additionally, we could provide an API for querying the currently selected asset.
// A texture viewer window could for example have the following default behaviour:
// - When a window has a texture target (e.g. drag-n-drop to set it), then keep previewing that texture.
// - When a window has no texture target, preview the currently selected asset (most recently pressed one if multiple asset windows open).
// That begs the question: what if the user wants an asset preview window that just previews the currently selected asset? OR what if the user wants one big "asset viewer" window where they can drag-n-drop any kind of asset to start viewing it? like a scene or a texture? So... I think Hatch should define an "AssetViewer" window type which does automatically preview selected assets as well.

// - EITHER we let you query the "currently selected" asset and automatically preview that, OR we do a drag-n-drop thing where
//   you can drag a texture asset into the texture viewer tab to begin previewing it. That option seems a lot better. Or we do both and have
//   an "asset preview" window and api.

#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// For D3D12
#define HT_INCLUDE_D3D12_API
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include "texture_viewer.inc.ht"

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
	params_type* params = HT_GetPluginData(params_type, ht);
	bool ok = ht->RegisterAssetViewerForType(params->texture_type);
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
	
	// Create PSO
	{
		ID3DBlob* vertex_shader;
		ID3DBlob* pixel_shader;

		UINT compile_flags = 0;
		
		static const char shader_src[] = "\
			struct PSInput {\
				float4 pos : SV_POSITION;\
				float2 uv : TEXCOORD0;\
			};\
			\
			PSInput VSMain(in uint vert_id : SV_VertexID)\
			{\
				float2 uv = float2((vert_id << 1) & 2, vert_id & 2);\
				PSInput output;\
				output.pos = float4(uv * 2.0 - float2(1.0, 1.0), 0.0, 1.0);\
				output.uv = uv;\
				return output;\
			}\
			\
			float4 PSMain(PSInput input) : SV_TARGET\
			{\
				return float4(frac(input.uv*5.0), 0.0, 1.0);\
			}\
			";
		
		ID3DBlob* errors = NULL;
		ok = ht->D3DCompile(shader_src, sizeof(shader_src), NULL, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, &errors) == S_OK;
		if (!ok) {
			ht->DebugPrint((char*)errors->GetBufferPointer());
			assert(0);
		}

		errors = NULL;
		ok = ht->D3DCompile(shader_src, sizeof(shader_src), NULL, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, &errors) == S_OK;
		if (!ok) {
			ht->DebugPrint((char*)errors->GetBufferPointer());
			assert(0);
		}
		
		if (ok) {
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
			if (!ok) ht->DebugPrint("Failed to create the graphics pipeline!\n");
		}
		
		vertex_shader->Release();
		pixel_shader->Release();
	}
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	GLOBALS.pipeline_state->Release();
	GLOBALS.root_signature->Release();
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
}

HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list) {
	HT_AssetViewerTabUpdate update;
	while (ht->PollNextAssetViewerTabUpdate(&update)) {
		D3D12_VIEWPORT viewport = {};
		viewport.TopLeftX = update.rect_min.x;
		viewport.TopLeftY = update.rect_min.y;
		viewport.Width = update.rect_max.x - update.rect_min.x;
		viewport.Height = update.rect_max.y - update.rect_min.y;
		command_list->RSSetViewports(1, &viewport);
		
		command_list->SetPipelineState(GLOBALS.pipeline_state);
		command_list->SetGraphicsRootSignature(GLOBALS.root_signature);
		
		command_list->DrawInstanced(3, 1, 0, 0);
	}
}
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
// ... do we need D3D12?? shouldn't hatch provide a texture upload API since it can draw textures?
/*#define HT_INCLUDE_D3D12_API
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>*/

// #include "texture_loader.inc.ht"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

#include <assert.h>

#define TODO() __debugbreak()

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

struct Globals {
	int _;
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
	GLOBALS.my_tab_class = ht->CreateTabClass("HLSLToy");
	
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
	
	MaybeHotreloadShader(ht);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
	ht->DestroyTabClass(GLOBALS.my_tab_class);

	GLOBALS.pipeline_state->Release();
	GLOBALS.root_signature->Release();
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	MaybeHotreloadShader(ht);
}

HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list) {
	if (GLOBALS.pipeline_state == NULL) return;
	
	// build draw commands.
	
	HT_TabUpdate tab_update;
	while (ht->PollNextTabUpdate(&tab_update)) {
		if (tab_update.tab_class == GLOBALS.my_tab_class) {
			D3D12_VIEWPORT viewport = {};
			viewport.TopLeftX = tab_update.rect_min.x;
			viewport.TopLeftY = tab_update.rect_min.y;
			viewport.Width = tab_update.rect_max.x - tab_update.rect_min.x;
			viewport.Height = tab_update.rect_max.y - tab_update.rect_min.y;
			command_list->RSSetViewports(1, &viewport);
			
			command_list->SetPipelineState(GLOBALS.pipeline_state);
			command_list->SetGraphicsRootSignature(GLOBALS.root_signature);
			
			command_list->DrawInstanced(3, 1, 0, 0);
		}
	}
}
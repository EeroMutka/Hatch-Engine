#ifdef HT_EDITOR_DX11

#include "include/ht_common.h"
#include "include/ht_editor_render.h"

#include <ht_utils/fire/fire_ui/fire_ui_backend_dx11.h>


EXPORT void RenderInit(RenderState* s, ivec2 window_size, OS_Window window) {
	s->window_size = window_size;

	D3D_FEATURE_LEVEL dx_feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };

	DXGI_SWAP_CHAIN_DESC swapchain_desc = {0};
	swapchain_desc.BufferDesc.Width  = 0; // use window width
	swapchain_desc.BufferDesc.Height = 0; // use window height
	swapchain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapchain_desc.SampleDesc.Count  = 8;
	swapchain_desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.BufferCount       = 2;
	swapchain_desc.OutputWindow      = (HWND)window.handle;
	swapchain_desc.Windowed          = TRUE;
	swapchain_desc.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;

	uint32_t create_device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
//#ifdef UI_DX11_DEBUG_MODE
		create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
//#endif

	HRESULT res = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
		create_device_flags, dx_feature_levels, ARRAYSIZE(dx_feature_levels), D3D11_SDK_VERSION,
		&swapchain_desc, &s->swapchain, &s->device, NULL, &s->dc);
	assert(res == S_OK);

	s->swapchain->GetDesc(&swapchain_desc); // Update swapchain_desc with actual window size

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3D11Texture2D* framebuffer;
	s->swapchain->GetBuffer(0, _uuidof(ID3D11Texture2D), (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc = {};
	framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	s->device->CreateRenderTargetView(framebuffer, &framebuffer_rtv_desc, &s->framebuffer_rtv);

	framebuffer->Release(); // We don't need this handle anymore
}

// for now we need to pass editor state to call HT_D3D12_BuildPluginCommandList on all plugins. TODO: cleanup this!
EXPORT void RenderEndFrame(EditorState* editor_state, RenderState* s, UI_Outputs* ui_outputs) {
	FLOAT clear_color[4] = { 0.15f, 0.15f, 0.15f, 1.f };
	s->dc->ClearRenderTargetView(s->framebuffer_rtv, clear_color);

	UI_DX11_Draw(ui_outputs, UI_VEC2{(float)s->window_size.x, (float)s->window_size.y}, s->framebuffer_rtv);
	
	// Reset scissor state after fire-UI might have messed with it
	D3D11_RECT scissor_rect = {};
	scissor_rect.right = s->window_size.x;
	scissor_rect.bottom = s->window_size.y;
	s->dc->RSSetScissorRects(1, &scissor_rect);
	
	D3D11_RenderPlugins(editor_state);
	
	s->swapchain->Present(1, 0);
}

EXPORT void ResizeSwapchain(RenderState* s, ivec2 size) {
	s->window_size = size;

	s->framebuffer_rtv->Release();

	s->swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* framebuffer;
	s->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc{};
	framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
	s->device->CreateRenderTargetView(framebuffer, &framebuffer_rtv_desc, &s->framebuffer_rtv);

	framebuffer->Release(); // We don't need this handle anymore

}

#endif
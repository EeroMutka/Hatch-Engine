#include "include/ht_common.h"
#include "include/ht_editor_render.h"

#include <ht_utils/fire/fire_ui/fire_ui_backend_dx12.h>

static void FreeRenderTargets(RenderState* s) {
    for (int i = 0; i < BACK_BUFFER_COUNT; i++) {
        s->back_buffers[i]->Release();
        s->back_buffers[i] = NULL;
    }
}

static void CreateRenderTargets(RenderState* s) {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = s->rtv_heap->GetCPUDescriptorHandleForHeapStart();

    // Create a RTV for each frame.
    for (int i = 0; i < BACK_BUFFER_COUNT; i++) {
        ID3D12Resource* back_buffer;
        bool ok = s->swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)) == S_OK;
        ASSERT(ok);

        s->device->CreateRenderTargetView(back_buffer, NULL, rtv_handle);
        s->back_buffers[i] = back_buffer;
        rtv_handle.ptr += s->rtv_descriptor_size;
    }
}

EXPORT void RenderInit(RenderState* s, ivec2 window_size, OS_Window window) {
    
//#ifdef UI_DX12_DEBUG_MODE

    ID3D12Debug* debug_interface = NULL;
    if (D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)) == S_OK) {
        debug_interface->EnableDebugLayer();
    }
//#endif

    s->window_size = window_size;

    bool ok = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&s->device)) == S_OK;
    ASSERT(ok);

//#ifdef UI_DX12_DEBUG_MODE
    if (debug_interface) {
        ID3D12InfoQueue* info_queue = NULL;
        s->device->QueryInterface(IID_PPV_ARGS(&info_queue));
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        info_queue->Release();
        debug_interface->Release();
    }
//#endif

	// Create queue
	{
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ok = s->device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&s->command_queue)) == S_OK;
		ASSERT(ok);
	}


	// Create descriptor heaps
	{
		// Describe and create a render target view (RTV) descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
		rtv_heap_desc.NumDescriptors = BACK_BUFFER_COUNT;
		rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		ok = s->device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&s->rtv_heap)) == S_OK;
		ASSERT(ok);

        D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
        srv_heap_desc.NumDescriptors = 1;
        srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ok = s->device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&s->srv_heap)) == S_OK;
        ASSERT(ok);

		s->rtv_descriptor_size = s->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create command allocator
	ok = s->device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&s->command_allocator)) == S_OK;
	ASSERT(ok);

    // Create the command list.
    ok = s->device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, s->command_allocator, NULL, IID_PPV_ARGS(&s->command_list)) == S_OK;
    ASSERT(ok);

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ok = s->command_list->Close() == S_OK;
    ASSERT(ok);

    // Create synchronization objects
    {
        ok = s->device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&s->fence)) == S_OK;
        ASSERT(ok);
        //s->render_finished_fence_value[0] = 1;
        //s->render_finished_fence_value[1] = 1;

        // Create an event handle to use for frame synchronization.
        s->fence_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        ASSERT(s->fence_event != NULL);
    }
    
    // Create swapchain
    {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.BufferCount = BACK_BUFFER_COUNT;
        swapchain_desc.Width = window_size.x;
        swapchain_desc.Height = window_size.y;
        swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.SampleDesc.Count = 1;

        IDXGIFactory4* dxgi_factory = NULL;
        ok = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)) == S_OK;
        ASSERT(ok);

        IDXGISwapChain1* swapchain1 = NULL;
        ok = dxgi_factory->CreateSwapChainForHwnd(s->command_queue, (HWND)window.handle, &swapchain_desc, NULL, NULL, &swapchain1) == S_OK;
        ASSERT(ok);

        ok = swapchain1->QueryInterface(IID_PPV_ARGS(&s->swapchain)) == S_OK;
        ASSERT(ok);

        swapchain1->Release();
        dxgi_factory->Release();
    }

    CreateRenderTargets(s);

	s->frame_index = s->swapchain->GetCurrentBackBufferIndex();
}

static D3D12_RESOURCE_BARRIER DX12Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES old_state, D3D12_RESOURCE_STATES new_state) {
    D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = old_state;
    barrier.Transition.StateAfter = new_state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return barrier;
}

EXPORT void ResizeSwapchain(RenderState* s, ivec2 size) {
	FreeRenderTargets(s);
	
	bool ok = s->swapchain->ResizeBuffers(0, size.x, size.y, DXGI_FORMAT_UNKNOWN, 0) == S_OK;
	ASSERT(ok);
	
	CreateRenderTargets(s);
	
	s->frame_index = s->swapchain->GetCurrentBackBufferIndex();
    s->window_size = size;
}

EXPORT void RenderEndFrame(EditorState* editor_state, RenderState* s, UI_Outputs* ui_outputs) {

	// Populate the command buffer
    {
        // Command list allocators can only be reset when the associated
        // command lists have finished execution on the GPU; apps should use
        // fences to determine GPU execution progress.
        bool ok = s->command_allocator->Reset() == S_OK;
        ASSERT(ok);

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ok = s->command_list->Reset(s->command_allocator, NULL) == S_OK;
        ASSERT(ok);

        D3D12_VIEWPORT viewport = {};
        viewport.Width = (float)s->window_size.x;
        viewport.Height = (float)s->window_size.y;
        s->command_list->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect = {};
        scissor_rect.right = s->window_size.x;
        scissor_rect.bottom = s->window_size.y;
        s->command_list->RSSetScissorRects(1, &scissor_rect);

        D3D12_RESOURCE_BARRIER present_to_rt = DX12Transition(s->back_buffers[s->frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        s->command_list->ResourceBarrier(1, &present_to_rt);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = s->rtv_heap->GetCPUDescriptorHandleForHeapStart();
        rtv_handle.ptr += s->frame_index * s->rtv_descriptor_size;
        s->command_list->OMSetRenderTargets(1, &rtv_handle, false, NULL);

        const float clear_color[] = { 0.15f, 0.15f, 0.15f, 1.0f };
        s->command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, NULL);

        s->command_list->SetDescriptorHeaps(1, &s->srv_heap);
        s->command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UI_DX12_Draw(ui_outputs, {(float)s->window_size.x, (float)s->window_size.y}, s->command_list);

        BuildPluginD3DCommandLists(editor_state);

        D3D12_RESOURCE_BARRIER rt_to_present = DX12Transition(s->back_buffers[s->frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        s->command_list->ResourceBarrier(1, &rt_to_present);

        ok = s->command_list->Close() == S_OK;
        ASSERT(ok);
    }

    ID3D12CommandList* command_lists[] = { s->command_list };
    s->command_queue->ExecuteCommandLists(DS_ArrayCount(command_lists), command_lists);

    bool ok = s->swapchain->Present(1, 0) == S_OK;
    ASSERT(ok);
	
	// Wait for GPU
	{
		// Wait for the GPU to finish rendering this frame.
		// This means that the CPU will have a bunch of idle time, and the GPU will have a bunch of idle time.
		// For computationally heavy applications, this is not good. But it's the simplest to implement.
		// There are some questions like how the dynamic texture atlas should be built if there are two
		// frames-in-flight - should there be two atlases, one per frame? Or should the CPU wait on frames where it modifies it?
		// I guess when only adding things to the atlas, no frame sync is even needed. This needs some thought.

		s->render_finished_fence_value++;

		uint64_t render_finished_fence_value = s->render_finished_fence_value;
		s->command_queue->Signal(s->fence, render_finished_fence_value);

		if (s->fence->GetCompletedValue() < s->render_finished_fence_value)
		{
			ok = s->fence->SetEventOnCompletion(s->render_finished_fence_value, s->fence_event) == S_OK;
			ASSERT(ok);
			WaitForSingleObjectEx(s->fence_event, INFINITE, FALSE);
		}

		s->frame_index = s->swapchain->GetCurrentBackBufferIndex();
	}
}

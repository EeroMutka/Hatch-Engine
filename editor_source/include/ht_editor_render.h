
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define BACK_BUFFER_COUNT 2

struct RenderState {
    ivec2 window_size;
    ID3D12Device* device;
    ID3D12CommandQueue* command_queue;
    IDXGISwapChain3* swapchain;
    ID3D12DescriptorHeap* rtv_heap;
    ID3D12DescriptorHeap* srv_heap;
    uint32_t rtv_descriptor_size;
    ID3D12Resource* back_buffers[BACK_BUFFER_COUNT];
    ID3D12CommandAllocator* command_allocator;

    ID3D12GraphicsCommandList* command_list;
    ID3D12Fence* fence;
    HANDLE fence_event;

    uint32_t frame_index;
    uint64_t render_finished_fence_value;
};

EXPORT void RenderInit(RenderState* s, ivec2 window_size, OS_WINDOW window);

EXPORT void RenderEndFrame(RenderState* s, UI_Outputs* ui_outputs);

EXPORT void ResizeSwapchain(RenderState* s, ivec2 size);

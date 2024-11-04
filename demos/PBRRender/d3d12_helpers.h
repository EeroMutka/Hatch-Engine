// This file is part of the Hatch codebase, written by Eero Mutka.
// For educational purposes only.

#include <stdint.h>
#include <assert.h>

// The amount of work required just to upload some data to a vertex buffer...
// whew.
struct TempGPUCmdContext {
	HT_API* ht;
	ID3D12CommandAllocator* cmd_allocator;
	ID3D12GraphicsCommandList* cmd_list;
	ID3D12Fence* fence;
	HANDLE fence_event;
	uint64_t fence_value;
	bool is_recording;
};

static TempGPUCmdContext CreateTempGPUCmdContext(HT_API* ht) {
	TempGPUCmdContext result = {};
	result.ht = ht;
	
	bool ok = ht->D3D_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&result.cmd_allocator)) == S_OK;
	assert(ok);
	
	ok = ht->D3D_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, result.cmd_allocator, NULL, IID_PPV_ARGS(&result.cmd_list)) == S_OK;
	assert(ok);
	
	ok = ht->D3D_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&result.fence)) == S_OK;
	assert(ok);
	
	result.fence_event = ht->D3DCreateEvent();
	assert(result.fence_event != NULL);
	
	return result;
}

static void DestroyTempGPUCmdContext(TempGPUCmdContext* ctx) {
	assert(!ctx->is_recording);
	
	ctx->ht->D3DDestroyEvent(ctx->fence_event);
	ctx->fence->Release();
	ctx->cmd_list->Release();
	ctx->cmd_allocator->Release();
}

static void BeginTempGPUCommands(TempGPUCmdContext* ctx) {
	assert(!ctx->is_recording);
	ctx->is_recording = true;
}

static void EndTempGPUCommands(TempGPUCmdContext* ctx) {
	assert(ctx->is_recording);
	ctx->is_recording = false;
	
	ctx->cmd_list->Close();
	
	ID3D12CommandList* command_lists[] = { ctx->cmd_list };
	ctx->ht->D3D_queue->ExecuteCommandLists(1, command_lists);
	
	ctx->fence_value += 1;
	ctx->ht->D3D_queue->Signal(ctx->fence, ctx->fence_value);
	
	// Wait for the fence to reach the new fence value
	bool ok = ctx->fence->SetEventOnCompletion(ctx->fence_value, ctx->fence_event) == S_OK;
	assert(ok);
	ctx->ht->D3DWaitForEvent(ctx->fence_event);
	
	ok = ctx->cmd_allocator->Reset() == S_OK;
	assert(ok);
	
	ok = ctx->cmd_list->Reset(ctx->cmd_allocator, NULL) == S_OK;
	assert(ok);
}

static D3D12_RESOURCE_BARRIER GPUTransition(ID3D12Resource* resource, D3D12_RESOURCE_STATES old_state, D3D12_RESOURCE_STATES new_state) {
    D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = old_state;
    barrier.Transition.StateAfter = new_state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return barrier;
}

static ID3D12Resource* CreateGPUBuffer(TempGPUCmdContext* ctx, u32 size, void* initial_data) {
	assert(initial_data != NULL);
	assert(!ctx->is_recording);
	
	D3D12_HEAP_PROPERTIES heap_props = {D3D12_HEAP_TYPE_DEFAULT};

	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ID3D12Resource* result;
	bool ok = ctx->ht->D3D_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL,
		IID_PPV_ARGS(&result)) == S_OK;
	assert(ok);
	
	D3D12_HEAP_PROPERTIES upload_heap_props = {D3D12_HEAP_TYPE_UPLOAD};
	
	ID3D12Resource* upload_buffer;
	ok = ctx->ht->D3D_device->CreateCommittedResource(&upload_heap_props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
		IID_PPV_ARGS(&upload_buffer)) == S_OK;
	assert(ok);

	void* mapped_ptr;
	ok = upload_buffer->Map(0, NULL, &mapped_ptr) == S_OK;
	assert(ok);
	memcpy(mapped_ptr, initial_data, size);
	upload_buffer->Unmap(0, NULL);
	
	// Upload data
	{
		BeginTempGPUCommands(ctx);
		
		ctx->cmd_list->CopyBufferRegion(result, 0, upload_buffer, 0, size);
		
		D3D12_RESOURCE_BARRIER copy_dest_to_read = GPUTransition(result, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
		ctx->cmd_list->ResourceBarrier(1, &copy_dest_to_read);
		
		EndTempGPUCommands(ctx);
	}
	
	upload_buffer->Release();
	
	return result;
}

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

static ID3D12Resource* CreateGPUTexture(TempGPUCmdContext* ctx, DXGI_FORMAT format, u32 width, u32 height, void* initial_data)
{
	ID3D12Resource* texture;
	ID3D12Resource* upload_buffer;
		
	D3D12_RESOURCE_DESC texture_desc = {};
	texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texture_desc.Alignment = 0;
	texture_desc.Width = width;
	texture_desc.Height = height;
	texture_desc.DepthOrArraySize = 1;
	texture_desc.MipLevels = 1;
	texture_desc.Format = format;
	texture_desc.SampleDesc.Count = 1;
	texture_desc.SampleDesc.Quality = 0;
	texture_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texture_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	
	D3D12_HEAP_PROPERTIES default_heap = {D3D12_HEAP_TYPE_DEFAULT};
	bool ok = ctx->ht->D3D_device->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COPY_DEST, NULL, IID_PPV_ARGS(&texture)) == S_OK;
	assert(ok);

	u64 unpadded_row_size;
	u64 required_upload_size;
	ctx->ht->D3D_device->GetCopyableFootprints(&texture_desc, 0, 1, 0, nullptr, nullptr, &unpadded_row_size, &required_upload_size);
	
	u32 row_size = DS_AlignUpPow2((u32)unpadded_row_size, D3D12_TEXTURE_DATA_PITCH_ALIGNMENT);
	assert(row_size == (u32)unpadded_row_size); // TODO

	D3D12_RESOURCE_DESC upload_buffer_desc = {};
	upload_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	upload_buffer_desc.Alignment = 0;
	upload_buffer_desc.Width = (u32)required_upload_size;
	upload_buffer_desc.Height = 1;
	upload_buffer_desc.DepthOrArraySize = 1;
	upload_buffer_desc.MipLevels = 1;
	upload_buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
	upload_buffer_desc.SampleDesc.Count = 1;
	upload_buffer_desc.SampleDesc.Quality = 0;
	upload_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	upload_buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
	D3D12_HEAP_PROPERTIES upload_heap = {D3D12_HEAP_TYPE_UPLOAD};
	ok = ctx->ht->D3D_device->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &upload_buffer_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&upload_buffer)) == S_OK;
	assert(ok);
	
	void* mapped_ptr;
	ok = upload_buffer->Map(0, NULL, &mapped_ptr) == S_OK;
	assert(ok);
	memcpy(mapped_ptr, initial_data, upload_buffer_desc.Width);
	upload_buffer->Unmap(0, NULL);
	
	// Upload data
	{
		BeginTempGPUCommands(ctx);
		
		D3D12_TEXTURE_COPY_LOCATION src_loc = {};
		src_loc.pResource = upload_buffer;
		src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src_loc.PlacedFootprint.Footprint.Format = format;
		src_loc.PlacedFootprint.Footprint.Width = width;
		src_loc.PlacedFootprint.Footprint.Height = height;
		src_loc.PlacedFootprint.Footprint.Depth = 1;
		src_loc.PlacedFootprint.Footprint.RowPitch = row_size;

		D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
		dst_loc.pResource = texture;
		dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst_loc.SubresourceIndex = 0;
		
		ctx->cmd_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, NULL);
		
		D3D12_RESOURCE_BARRIER copy_dest_to_ps_resource = GPUTransition(texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		ctx->cmd_list->ResourceBarrier(1, &copy_dest_to_ps_resource);
		
		EndTempGPUCommands(ctx);
	}
	
	upload_buffer->Release();
	return texture;
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

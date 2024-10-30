#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

#include "test_plugin.hatch_generated.h"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

// This should be in the utility libraries...
#define UI_API static
#include "../editor_source/fire/fire_ui/fire_ui.c"

//#include "utility_libraries/hatch_ui.h"

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

#define TODO() __debugbreak()

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

struct UIBackend {
	HT_API* ht;
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

// -- GLOBALS ---------------

static UIBackend G_UI;

// --------------------------

static UI_CachedGlyph UIGetCachedGlyph(u32 codepoint, UI_Font font) {
	HT_CachedGlyph glyph = G_UI.ht->GetCachedGlyph(codepoint);
	return *(UI_CachedGlyph*)&glyph;
}

static UI_DrawVertex* UIResizeAndMapVertexBuffer(int num_vertices) {
	DS_ArrResizeUndef(&G_UI.vertex_buffer, num_vertices);
	return G_UI.vertex_buffer.data;
}

static u32* UIResizeAndMapIndexBuffer(int num_indices) {
	DS_ArrResizeUndef(&G_UI.index_buffer, num_indices);
	return G_UI.index_buffer.data;
}

// --------------------------

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// ht->DebugPrint("This is NOT fun!\n");
	
	// hmm... so ideally there would be a duplicate version of fire UI I guess?
	// the question is... how much access do we want to give plugins?
	
	// I think we want to use fire_ui with a special "hatch" backend. We don't want hatch_api to expose a fire_ui state struct.
	// The downside is that the glyph atlas would be duplicated across any plugins that require UI. Not good.
	// So in the hatch API we need to provide access to the fonts atlas and the default font. And we should let
	// you easily allocate and free atlas texture regions.
	
	// Ok, so can we make a fire UI button using a local copy of the fire-UI library?
	// maybe we should first solve the atlas thing yeah...
	
	
	// ok... so first thing first, we need to allocate memory.
	// Then we need fire_ds support.
	
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	
	DS_DynArray(int) my_things = {temp_allocator};
	DS_ArrPush(&my_things, 1);
	DS_ArrPush(&my_things, 2);
	DS_ArrPush(&my_things, 3);
	DS_ArrPush(&my_things, 4);
	
	// ht->DrawText("Hello!", {400, 500}, HT_AlignH_Left, 100, {255, 0, 255, 255});

	UI_Init(temp_allocator);
	
	// setup UI render backend
	G_UI.ht = ht;
	DS_ArrInit(&G_UI.vertex_buffer, temp_allocator);
	DS_ArrInit(&G_UI.index_buffer, temp_allocator);
	UI_STATE.backend.ResizeAndMapVertexBuffer = UIResizeAndMapVertexBuffer;
	UI_STATE.backend.ResizeAndMapIndexBuffer  = UIResizeAndMapIndexBuffer;
	
	// setup UI text rendering backend
	UI_STATE.backend.GetCachedGlyph = UIGetCachedGlyph;
	
	UI_Inputs ui_inputs = {};
	UI_BeginFrame(&ui_inputs, {0, 18});
	
	UI_DrawCircle({500, 500}, 100.f, 12, {0, 255, 0, 255});
	UI_DrawCircle({200, 400}, 50.f, 16, {255, 255, 0, 255});

	// Submit UI render commands to hatch
	UI_Outputs ui_outputs;
	UI_EndFrame(&ui_outputs);
	
	u32 first_vertex;
	HT_DrawVertex* ht_vertices = ht->AddVertices(UI_STATE.vertex_buffer_count, &first_vertex);
	memcpy(ht_vertices, G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count * sizeof(HT_DrawVertex));
	
	for (int i = 0; i < ui_outputs.draw_commands_count; i++) {
		UI_DrawCommand* draw_command = &ui_outputs.draw_commands[i];
		
		u32* indices = ht->AddIndices(draw_command->index_count, NULL);
		memcpy(indices, &G_UI.index_buffer[draw_command->first_index], draw_command->index_count * sizeof(u32));
		
		u32* end = indices + draw_command->index_count;
		for (u32* idx = indices; idx < end; idx++) {
			*idx += first_vertex;
		}
	}
}

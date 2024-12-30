#define HT_STATIC_PLUGIN_ID SceneEdit

#include <hatch_api.h>
#include "../SceneEdit.inc.ht"

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>

#include <ht_utils/input/input.h>

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>

#include <ht_utils/gizmos/gizmos.h>

#include "camera.h"
#include "scene_edit.h"

#define TODO() __debugbreak()

// -----------------------------------------------------

struct Globals {
	HT_TabClass* my_tab_class;
};

struct UIBackend {
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

// -----------------------------------------------------

static SceneEditState G_STATE;
static Globals GLOBALS;
static UIBackend G_UI;
static HT_API* G_HT;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = G_HT->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

static UI_CachedGlyph UIGetCachedGlyph(u32 codepoint, UI_Font font) {
	HT_CachedGlyph glyph = G_HT->GetCachedGlyph(codepoint);
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

// -----------------------------------------------------

static void DebugSceneTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
	G_HT = ht;

	// -----------------------------------------------

	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	HT_ASSERT(scene);
	
	SceneEditUpdate(ht, &G_STATE, update_info->rect, ht->input_frame->mouse_position, scene);

	// -----------------------------------------------

	DS_Arena temp_arena = {0};
	DS_Info ds = { &temp_arena };
	DS_AllocatorBase temp_allocator = { &ds, TempAllocatorProc };
	DS_ArenaInit(&temp_arena, 4096, (DS_Allocator*)&temp_allocator);	
	UI_Init(&temp_arena);
	
	// init UI
	{
		DS_ArrInit(&G_UI.vertex_buffer, &temp_arena);
		DS_ArrInit(&G_UI.index_buffer, &temp_arena);
		UI_STATE.backend.ResizeAndMapVertexBuffer = UIResizeAndMapVertexBuffer;
		UI_STATE.backend.ResizeAndMapIndexBuffer  = UIResizeAndMapIndexBuffer;
		UI_STATE.backend.GetCachedGlyph = UIGetCachedGlyph;
		
		UI_Inputs ui_inputs = {};
		UI_BeginFrame(&ui_inputs, {0, 18}, {0, 18});
	}
	
	SceneEditDrawGizmos(ht, &G_STATE, scene);

	// -----------------------------------------------
	
	// Submit UI render commands to hatch
	UI_FinalizeDrawBatch();
	
	u32 first_vertex = ht->AddVertices((HT_DrawVertex*)G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count);
	for (int i = 0; i < G_UI.index_buffer.count; i++) {
		G_UI.index_buffer.data[i] += first_vertex;
	}
	
	for (int i = 0; i < UI_STATE.draw_commands.count; i++) {
		UI_DrawCommand* draw_command = &UI_STATE.draw_commands.data[i];
		ht->AddIndices(&G_UI.index_buffer[draw_command->first_index], draw_command->index_count);
	}
}

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	// hmm... so maybe SceneEdit can provide an API for unlocking the asset viewer registration.
	// That way, by default it can take the asset viewer, but it also specifies that others can take it if they want to.
	
	// bool ok = ht->RegisterAssetViewerForType(ht->types->Scene__Scene, DebugSceneTabUpdate);
	// HT_ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// OR: maybe the scene edit plugin can act more like a library for the renderer plugin.
	// The renderer plugin can then call functions from the scene edit plugin, like "UpdateScene" and "GenerateGizmos".
	// This sounds good, because it lets us use our current simplistic hatch setup. Let's do it.
}

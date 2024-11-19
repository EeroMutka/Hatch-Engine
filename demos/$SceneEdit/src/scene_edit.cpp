#include <hatch_api.h>
#include "../SceneEdit.inc.ht"

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

#define UI_API static
#define UI_CUSTOM_VEC2
#define UI_Vec2 vec2
#include <ht_utils/fire/fire_ui/fire_ui.c>

#include <ht_utils/gizmos/gizmos.h>

#define ASSERT(x) do { if (!(x)) __debugbreak(); } while(0)
#define TODO() __debugbreak()

// -----------------------------------------------------

struct Globals {
	HT_TabClass* my_tab_class;
};

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

struct UIBackend {
	HT_API* ht;
	DS_DynArray(UI_DrawVertex) vertex_buffer;
	DS_DynArray(u32) index_buffer;
};

// -----------------------------------------------------

static Globals GLOBALS;
static UIBackend G_UI;

// -----------------------------------------------------

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

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

// -----------------------------------------------------

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	SceneEditParams* params = HT_GetPluginData(SceneEditParams, ht);
	ASSERT(params);

	// hmm... so maybe SceneEdit can provide an API for unlocking the asset viewer registration.
	// That way, by default it can take the asset viewer, but it also specifies that others can take it if they want to.
	bool ok = ht->RegisterAssetViewerForType(params->scene_type);
	ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// OR: maybe the scene edit plugin can act more like a library for the renderer plugin.
	// The renderer plugin can then call functions from the scene edit plugin, like "UpdateScene" and "GenerateGizmos".
	// This sounds good, because it lets us use our current simplistic hatch setup. Let's do it.
	// THOUGH... if we do it library-style, then the code will be duplicated. Maybe we need a way to expose DLL calls in the future!
	
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	
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
	UI_BeginFrame(&ui_inputs, {0, 18}, {0, 18});
	
	HT_AssetViewerTabUpdate update;
	while (ht->PollNextAssetViewerTabUpdate(&update)) {
		vec2 rect_min = {(float)update.rect_min.x, (float)update.rect_min.y}; // TODO: it would be nice to support implicit conversion!
		vec2 rect_max = {(float)update.rect_max.x, (float)update.rect_max.y};
		
		//vec3 camera_pos = {0.f, 0.f, -10.f};
		// mat4 cs_from_ws = M_Perspective_LH_ZO(M_AngleDeg(70.f), 1.f, 0.01f, 100.f);// * M_Translate(-camera_pos);
		
		mat4 ws_to_cs = M_MakePerspectiveMatrixSimple(5.f); // * M_Translate(-camera_pos);
		
		GizmosViewport vp;
		vp.camera.position = {0, 0, 0};
		vp.camera.ws_to_cs = ws_to_cs;
		// vp.camera.cs_to_ws = M_InvGeneralM4(cs_from_ws);
		vp.window_size = {rect_max - rect_min};
		vp.window_size_inv = {1.f / vp.window_size.x, 1.f / vp.window_size.y};
		
		DrawCuboid3D(&vp, vec3{-1, -1, -1+10}, vec3{1, 1, 1+10}, 5.f, UI_BLUE);
		
		vec2 middle = (rect_min + rect_max) * 0.5f;
		UI_DrawCircle(middle, 100.f, 12, {0, 255, 0, 255});
	}
	
	// UI_DrawCircle({200, 400}, 50.f, 16, {255, 255, 0, 255});

	// Submit UI render commands to hatch
	UI_Outputs ui_outputs;
	UI_EndFrame(&ui_outputs);
	
	u32 first_vertex = ht->AddVertices((HT_DrawVertex*)G_UI.vertex_buffer.data, UI_STATE.vertex_buffer_count);
	
	int ib_count = G_UI.index_buffer.count;
	for (int i = 0; i < ib_count; i++) {
		G_UI.index_buffer.data[i] += first_vertex;
	}
	
	for (int i = 0; i < ui_outputs.draw_commands_count; i++) {
		UI_DrawCommand* draw_command = &ui_outputs.draw_commands[i];
		ht->AddIndices(&G_UI.index_buffer[draw_command->first_index], draw_command->index_count);
	}
}

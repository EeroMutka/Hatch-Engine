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

static void DebugSceneTabUpdate(HT_API* ht, const HT_AssetViewerTabUpdate* update_info) {
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
	
	// -----------------------------------------------
	
	vec2 rect_min = {(float)update_info->rect_min.x, (float)update_info->rect_min.y}; // TODO: it would be nice to support implicit conversion!
	vec2 rect_max = {(float)update_info->rect_max.x, (float)update_info->rect_max.y};
	
	Scene__Scene* scene = HT_GetAssetData(Scene__Scene, ht, update_info->data_asset);
	HT_ASSERT(scene);
	
	SceneEdit__SceneEditParams* params = HT_GetPluginData(SceneEdit__SceneEditParams, ht);
	HT_ASSERT(params);
	
	Camera camera = SceneEditUpdate(ht, scene, params->editor_camera_type);
	
	vec2 rect_size = rect_max - rect_min;
	vec2 rect_middle = (rect_min + rect_max) * 0.5f;
	mat4 cs_to_ss = M_MatScale(vec3{0.5f*rect_size.x, 0.5f*rect_size.y, 1.f}) * M_MatTranslate(vec3{rect_middle.x, rect_middle.y, 0});
	
	mat4 ws_to_cs = camera.ws_to_vs * M_MakePerspectiveMatrix(M_DegToRad*70.f, rect_size.x / rect_size.y, 0.1f, 100.f);
	mat4 ws_to_ss = ws_to_cs * cs_to_ss;
	
	GizmosViewport vp;
	vp.camera.position = {0, 0, 0};
	vp.camera.ws_to_ss = ws_to_ss;
	
	DrawGrid3D(&vp, UI_GRAY);
	DrawArrow3D(&vp, {}, vec3{1.f, 0.f, 0.f}, 0.03f, 0.012f, 12, 5.f, UI_RED);
	DrawArrow3D(&vp, {}, vec3{0.f, 1.f, 0.f}, 0.03f, 0.012f, 12, 5.f, UI_RED);
	DrawArrow3D(&vp, {}, vec3{0.f, 0.f, 1.f}, 0.03f, 0.012f, 12, 5.f, UI_RED);
	
	for (HT_ItemGroupEach(&scene->entities, i)) {
		Scene__SceneEntity* entity = HT_GetItem(Scene__SceneEntity, &scene->entities, i);
		
		for (int comp_i = 0; comp_i < entity->components.count; comp_i++) {
			HT_Any* comp_any = &((HT_Any*)entity->components.data)[comp_i];
			
			if (comp_any->type.kind == HT_TypeKind_Struct && comp_any->type._struct == params->box_component_type) {
				Scene__BoxComponent* box_component = (Scene__BoxComponent*)comp_any->data;
				
				vp.camera.ws_to_ss =
					M_MatScale(entity->scale) *
					M_MatRotateX(entity->rotation.x*M_DegToRad) *
					M_MatRotateY(entity->rotation.y*M_DegToRad) *
					M_MatRotateZ(entity->rotation.z*M_DegToRad) *
					M_MatTranslate(entity->position) *
					ws_to_ss;
				
				DrawCuboid3D(&vp,
					box_component->half_extent * -1.f,
					box_component->half_extent,
					5.f,
					UI_GOLD);
				vp.camera.ws_to_ss = ws_to_ss;
			}
		}
	}
	
	// -----------------------------------------------
	
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

HT_EXPORT void HT_LoadPlugin(HT_API* ht) {
	SceneEdit__SceneEditParams* params = HT_GetPluginData(SceneEdit__SceneEditParams, ht);
	HT_ASSERT(params);

	// hmm... so maybe SceneEdit can provide an API for unlocking the asset viewer registration.
	// That way, by default it can take the asset viewer, but it also specifies that others can take it if they want to.
	
	bool ok = ht->RegisterAssetViewerForType(params->scene_type, DebugSceneTabUpdate);
	HT_ASSERT(ok);
}

HT_EXPORT void HT_UnloadPlugin(HT_API* ht) {
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// OR: maybe the scene edit plugin can act more like a library for the renderer plugin.
	// The renderer plugin can then call functions from the scene edit plugin, like "UpdateScene" and "GenerateGizmos".
	// This sounds good, because it lets us use our current simplistic hatch setup. Let's do it.
}

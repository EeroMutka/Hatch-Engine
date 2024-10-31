#include "include/ht_internal.h"
#include "include/ht_editor_render.h"

#include "fire/fire_ui/fire_ui_backend_fire_os.h"
#include "fire/fire_ui/fire_ui_backend_dx12.h"
#include "fire/fire_ui/fire_ui_backend_stb_truetype.h"

// -- Globals -----------------------------

EXPORT DS_Arena* TEMP;

extern "C" {
	EXPORT UI_State UI_STATE;
	EXPORT UI_DX12_State UI_DX12_STATE;
	EXPORT UI_STBTT_State UI_STBTT_STATE;
}

// ----------------------------------------

static STR_View QueryTabName(UI_PanelTree* tree, UI_Tab* tab) {
	switch (tab->kind) {
	case TabKind_Assets:     return "Assets";
	case TabKind_Properties: return "Properties";
	case TabKind_Log:        return "Log";
	}
	return "";
}

void UpdateAndDrawTab(UI_PanelTree* tree, UI_Tab* tab, UI_Key key, UI_Rect area_rect) {
	EditorState* s = (EditorState*)tree->user_data;
	switch (tab->kind) {
	case TabKind_Assets:     UIAssetsBrowserTab(s, key, area_rect); break;
	case TabKind_Properties: UIPropertiesTab(s, key, area_rect); break;
	case TabKind_Log:        UILogTab(s, key, area_rect); break;
	}
}

static void AppInit(EditorState* s) {
	DS_ArenaInit(&s->persistent_arena, 4096, DS_HEAP);
	DS_ArenaInit(&s->temporary_arena, 4096, DS_HEAP);

	DS_Arena* persist = &s->persistent_arena;

	s->window = OS_WINDOW_Create(s->window_size.x, s->window_size.y, "Hatch");
	
	RenderInit(s->render_state, s->window_size, s->window);

	D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor = s->render_state->srv_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor = s->render_state->srv_heap->GetGPUDescriptorHandleForHeapStart();

	UI_Init(DS_HEAP);
	UI_DX12_Init(s->render_state->device, atlas_cpu_descriptor, atlas_gpu_descriptor);
	UI_STBTT_Init(UI_DX12_CreateAtlas, UI_DX12_MapAtlas);

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR_View roboto_mono_ttf, icons_ttf;
	OS_ReadEntireFile(MEM_SCOPE(persist), "../editor_source/fire/fire_ui/resources/roboto_mono.ttf", &roboto_mono_ttf);
	OS_ReadEntireFile(MEM_SCOPE(persist), "../editor_source/fire/fire_ui/resources/fontello/font/fontello.ttf", &icons_ttf);

	s->default_font = { UI_STBTT_FontInit(roboto_mono_ttf.data, -4.f), 18 };
	s->icons_font = { UI_STBTT_FontInit(icons_ttf.data, -2.f), 18 };

	// -- Hatch stuff ---------------------------------------------------------------------------

	STR_View exe_path;
	OS_GetThisExecutablePath(MEM_SCOPE(persist), &exe_path);
	s->hatch_install_directory = STR_BeforeLast(STR_BeforeLast(exe_path, '\\'), '\\');
	
	UI_PanelTreeInit(&s->panel_tree, DS_HEAP);
	s->panel_tree.query_tab_name = QueryTabName;
	s->panel_tree.update_and_draw_tab = UpdateAndDrawTab;
	s->panel_tree.user_data = s;
	
	DS_SlotAllocatorInit(&s->asset_tree.assets, persist);
	s->asset_tree.next_asset_generation = 1;
	s->asset_tree.root = (Asset*)DS_TakeSlot(&s->asset_tree.assets);
	*s->asset_tree.root = {};

	s->panel_tree.root = NewUIPanel(&s->panel_tree);
	UI_Panel* root_left_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* log_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* asset_browser_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* properties_panel = NewUIPanel(&s->panel_tree);

	static UI_Tab asset_browser_tab = {TabKind_Assets};
	DS_ArrPush(&asset_browser_panel->tabs, &asset_browser_tab);
	
	static UI_Tab log_tab = {TabKind_Log};
	DS_ArrPush(&log_panel->tabs, &log_tab);

	static UI_Tab properties_tab = {TabKind_Properties};
	DS_ArrPush(&properties_panel->tabs, &properties_tab);

	s->panel_tree.root->split_along = UI_Axis_Y;
	s->panel_tree.root->end_child[0] = root_left_panel;
	s->panel_tree.root->end_child[1] = properties_panel;
	root_left_panel->parent = s->panel_tree.root;
	properties_panel->parent = s->panel_tree.root;
	root_left_panel->link[1] = properties_panel;
	properties_panel->link[0] = root_left_panel;

	root_left_panel->split_along = UI_Axis_X;
	root_left_panel->end_child[0] = asset_browser_panel;
	root_left_panel->end_child[1] = log_panel;
	asset_browser_panel->parent = root_left_panel;
	log_panel->parent = root_left_panel;
	asset_browser_panel->link[1] = log_panel;
	log_panel->link[0] = asset_browser_panel;

	DS_ArenaInit(&s->log_arenas[0], DS_KIB(16), DS_HEAP);
	DS_ArenaInit(&s->log_arenas[1], DS_KIB(16), DS_HEAP);
	DS_ArrInit(&s->log_messages[0], &s->log_arenas[0]);
	DS_ArrInit(&s->log_messages[1], &s->log_arenas[1]);

	UI_TextInit(DS_HEAP, &s->dummy_text, "");
	UI_TextInit(DS_HEAP, &s->dummy_text_2, "");

	{
		s->asset_tree.name_and_type_struct_type = MakeNewAsset(&s->asset_tree, AssetKind_StructType);
		
		StructMember name_member = {0};
		UI_TextInit(DS_HEAP, &name_member.name.text, "Name");
		name_member.type.kind = TypeKind_String;
		DS_ArrPush(&s->asset_tree.name_and_type_struct_type->struct_type.members, name_member);

		StructMember type_member = {0};
		UI_TextInit(DS_HEAP, &type_member.name.text, "Type");
		type_member.type.kind = TypeKind_Type;
		DS_ArrPush(&s->asset_tree.name_and_type_struct_type->struct_type.members, type_member);
		
		ComputeStructLayout(s->asset_tree.name_and_type_struct_type);

		s->asset_tree.plugin_options_struct_type = MakeNewAsset(&s->asset_tree, AssetKind_StructType);

		StructMember member_source_files = {0};
		UI_TextInit(DS_HEAP, &member_source_files.name.text, "SourceFiles");
		member_source_files.type.kind = TypeKind_Array;
		member_source_files.type.subkind = TypeKind_AssetRef;
		DS_ArrPush(&s->asset_tree.plugin_options_struct_type->struct_type.members, member_source_files);

		StructMember member_data = {0};
		UI_TextInit(DS_HEAP, &member_data.name.text, "Data");
		member_data.type.kind = TypeKind_AssetRef;
		DS_ArrPush(&s->asset_tree.plugin_options_struct_type->struct_type.members, member_data);
		
		ComputeStructLayout(s->asset_tree.plugin_options_struct_type);
	}

	InitAPI(s);
}

static void UpdateAndDraw(EditorState* s) {
	// {s->icons_font, 18}
	UI_BeginFrame(&s->ui_inputs, s->default_font);

	s->frame = {};
	
	UIDropdownStateBeginFrame(&s->dropdown_state);

	UI_Box* root_box = UI_BOX();
	UI_InitRootBox(root_box, (float)s->window_size.x, (float)s->window_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root_box);
	UI_PushBox(root_box);

	AddTopBar(s);

	UI_PopBox(root_box);
	UI_BoxComputeRects(root_box, vec2{0, 0});
	UI_DrawBox(root_box);

	UI_Rect panel_area_rect = {0};
	panel_area_rect.min.y = TOP_BAR_HEIGHT;
	panel_area_rect.max = UI_VEC2{(float)s->window_size.x, (float)s->window_size.y};
	UI_PanelTreeUpdateAndDraw(&s->panel_tree, s->panel_tree.root, panel_area_rect, false, s->icons_font, &s->frame.hovered_panel);
	
	UpdatePlugins(s);

	UpdateAndDrawDropdowns(s);

	UI_Outputs ui_outputs;
	UI_EndFrame(&ui_outputs);
	
	UI_STBTT_FreeUnusedGlyphs();
	
	UI_OS_ApplyOutputs(&s->window, &ui_outputs);
	
	RenderEndFrame(s, s->render_state, &ui_outputs);
}

static void OnResizeWindow(u32 width, u32 height, void* user_ptr) {
	EditorState* s = (EditorState*)user_ptr;
	s->window_size = {(int)width, (int)height};
	
	ResizeSwapchain(s->render_state, s->window_size);

	UpdateAndDraw(s);
}

int main() {
	EditorState editor_state = {};
	RenderState render_state = {};
	
	editor_state.render_state = &render_state;
	TEMP = &editor_state.temporary_arena;

	AppInit(&editor_state);

	for (;;) {
		DS_ArenaReset(&editor_state.temporary_arena);
		UI_OS_ResetFrameInputs(&editor_state.window, &editor_state.ui_inputs);

		OS_WINDOW_Event event;
		while (OS_WINDOW_PollEvent(&editor_state.window, &event, OnResizeWindow, &editor_state)) {
			UI_OS_RegisterInputEvent(&editor_state.ui_inputs, &event);
		}
		if (OS_WINDOW_ShouldClose(&editor_state.window)) break;

		UpdateAndDraw(&editor_state);
	}

	return 0;
}

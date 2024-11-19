#include "include/ht_common.h"
#include "include/ht_editor_render.h"

#include <ht_utils/fire/fire_ui/fire_ui_backend_fire_os.h>
#include <ht_utils/fire/fire_ui/fire_ui_backend_dx12.h>
#include <ht_utils/fire/fire_ui/fire_ui_backend_stb_truetype.h>

// -- Globals -----------------------------

EXPORT DS_Arena* TEMP;

extern "C" {
	EXPORT UI_State UI_STATE;
	EXPORT DS_Arena* UI_TEMP;
	EXPORT UI_DX12_State UI_DX12_STATE;
	EXPORT UI_STBTT_State UI_STBTT_STATE;
}

// ----------------------------------------

static STR_View QueryTabName(UI_PanelTree* tree, UI_Tab* tab) {
	return tab->name;
}

void UpdateAndDrawTab(UI_PanelTree* tree, UI_Tab* tab, UI_Key key, UI_Rect area_rect) {
	EditorState* s = (EditorState*)tree->user_data;
	if (tab == s->assets_tab_class) {
		UpdateAndDrawAssetsBrowserTab(s, key, area_rect);
	}
	else if (tab == s->properties_tab_class) {
		UpdateAndDrawPropertiesTab(s, key, area_rect);
	}
	else if (tab == s->log_tab_class) {
		UpdateAndDrawLogTab(s, key, area_rect);
	}
	else if (tab == s->asset_viewer_tab_class) {
		HT_Asset selected_asset = (HT_Asset)s->assets_tree_ui_state.selection;
		if (AssetIsValid(&s->asset_tree, selected_asset)) {
			HT_AssetViewerTabUpdate update = {};
			update.data_asset = selected_asset;
			update.rect_min = {(int)area_rect.min.x, (int)area_rect.min.y};
			update.rect_max = {(int)area_rect.max.x, (int)area_rect.max.y};
			DS_ArrPush(&s->frame.queued_asset_viewer_tab_updates, update);
		}
	}
	else {
		HT_CustomTabUpdate update;
		update.tab_class = (HT_TabClass*)tab;
		update.rect_min = {(int)area_rect.min.x, (int)area_rect.min.y};
		update.rect_max = {(int)area_rect.max.x, (int)area_rect.max.y};
		DS_ArrPush(&s->frame.queued_custom_tab_updates, update);
	}
}



static void AppInit(EditorState* s) {
	DS_ArenaInit(&s->persistent_arena, 4096, DS_HEAP);
	DS_ArenaInit(&s->temporary_arena, 4096, DS_HEAP);

	DS_Arena* persist = &s->persistent_arena;

	s->window = OS_CreateWindow(s->window_size.x, s->window_size.y, "Hatch");
	
	RenderInit(s->render_state, s->window_size, s->window);

	D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor = s->render_state->srv_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor = s->render_state->srv_heap->GetGPUDescriptorHandleForHeapStart();

	UI_Init(DS_HEAP);
	UI_DX12_Init(s->render_state->device, atlas_cpu_descriptor, atlas_gpu_descriptor);
	UI_STBTT_Init(UI_DX12_CreateAtlas, UI_DX12_MapAtlas);

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR_View roboto_mono_ttf, icons_ttf;
	OS_ReadEntireFile(MEM_SCOPE(persist), "../plugin_include/ht_utils/fire/fire_ui/resources/roboto_mono.ttf", &roboto_mono_ttf);
	OS_ReadEntireFile(MEM_SCOPE(persist), "../plugin_include/ht_utils/fire/fire_ui/resources/fontello/font/fontello.ttf", &icons_ttf);

	s->default_font = { UI_STBTT_FontInit(roboto_mono_ttf.data, -4.f), 18 };
	s->icons_font = { UI_STBTT_FontInit(icons_ttf.data, -2.f), 18 };

	// -- Hatch stuff ---------------------------------------------------------------------------

	DS_ArenaInit(&s->log_arena, 4096, DS_HEAP);
	DS_ArrInit(&s->log_messages, &s->log_arena);

	STR_View exe_path;
	OS_GetThisExecutablePath(MEM_SCOPE(persist), &exe_path);
	s->hatch_install_directory = STR_BeforeLast(STR_BeforeLast(exe_path, '\\'), '\\');
	
	UI_PanelTreeInit(&s->panel_tree, DS_HEAP);
	s->panel_tree.query_tab_name = QueryTabName;
	s->panel_tree.update_and_draw_tab = UpdateAndDrawTab;
	s->panel_tree.user_data = s;
	
	DS_BucketArrayInit(&s->tab_classes, persist, 16);
	s->assets_tab_class = CreateTabClass(s, "Assets");
	s->log_tab_class = CreateTabClass(s, "Log");
	s->properties_tab_class = CreateTabClass(s, "Properties");
	s->asset_viewer_tab_class = CreateTabClass(s, "Asset Viewer");

	DS_MapInit(&s->asset_tree.package_from_name, DS_HEAP);
	DS_ArrInit(&s->asset_tree.asset_buckets, DS_HEAP);
	s->asset_tree.first_free_asset.val = ASSET_FREELIST_END;
	s->asset_tree.root = MakeNewAsset(&s->asset_tree, AssetKind_Root);

	s->panel_tree.root = NewUIPanel(&s->panel_tree);
	UI_Panel* root_left_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* log_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* asset_browser_panel = NewUIPanel(&s->panel_tree);
	UI_Panel* properties_panel = NewUIPanel(&s->panel_tree);

	DS_ArrPush(&asset_browser_panel->tabs, s->assets_tab_class);
	DS_ArrPush(&log_panel->tabs, s->asset_viewer_tab_class);
	DS_ArrPush(&log_panel->tabs, s->log_tab_class);
	DS_ArrPush(&properties_panel->tabs, s->properties_tab_class);
	
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

	UI_TextInit(DS_HEAP, &s->dummy_text, "");
	UI_TextInit(DS_HEAP, &s->dummy_text_2, "");

	{
		s->asset_tree.name_and_type_struct_type = MakeNewAsset(&s->asset_tree, AssetKind_StructType);
		
		StructMember name_member = {0};
		UI_TextInit(DS_HEAP, &name_member.name.text, "Name");
		name_member.type.kind = HT_TypeKind_String;
		DS_ArrPush(&s->asset_tree.name_and_type_struct_type->struct_type.members, name_member);

		StructMember type_member = {0};
		UI_TextInit(DS_HEAP, &type_member.name.text, "HT_Type");
		type_member.type.kind = HT_TypeKind_Type;
		DS_ArrPush(&s->asset_tree.name_and_type_struct_type->struct_type.members, type_member);
		
		ComputeStructLayout(&s->asset_tree, s->asset_tree.name_and_type_struct_type);

		s->asset_tree.plugin_options_struct_type = MakeNewAsset(&s->asset_tree, AssetKind_StructType);

		StructMember member_source_files = {0};
		UI_TextInit(DS_HEAP, &member_source_files.name.text, "Source Files");
		member_source_files.type.kind = HT_TypeKind_Array;
		member_source_files.type.subkind = HT_TypeKind_AssetRef;
		DS_ArrPush(&s->asset_tree.plugin_options_struct_type->struct_type.members, member_source_files);

		StructMember member_data = {0};
		UI_TextInit(DS_HEAP, &member_data.name.text, "Data Asset");
		member_data.type.kind = HT_TypeKind_AssetRef;
		DS_ArrPush(&s->asset_tree.plugin_options_struct_type->struct_type.members, member_data);
		
		ComputeStructLayout(&s->asset_tree, s->asset_tree.plugin_options_struct_type);
	}

	STR_View startup_project_file = STR_Form(TEMP, "%v/user_startup_project/.htproject", s->hatch_install_directory);
	LoadProject(&s->asset_tree, startup_project_file);

	InitAPI(s);
}

static void UpdateAndDraw(EditorState* s) {
	UI_BeginFrame(&s->ui_inputs, s->default_font, s->icons_font);

	s->frame = {};
	DS_ArrInit(&s->frame.queued_custom_tab_updates, TEMP);
	DS_ArrInit(&s->frame.queued_asset_viewer_tab_updates, TEMP);
	
	UIDropdownStateBeginFrame(&s->dropdown_state);
	
	//if (UI_InputIsDown(UI_Input_Shift)) __debugbreak();

	HotreloadPackages(&s->asset_tree);

	UI_Rect panel_area_rect = {0};
	panel_area_rect.min.y = TOP_BAR_HEIGHT;
	panel_area_rect.max = UI_VEC2{(float)s->window_size.x, (float)s->window_size.y};
	UI_PanelTreeUpdateAndDraw(&s->dropdown_state, &s->panel_tree, s->panel_tree.root, panel_area_rect, false, s->icons_font, &s->frame.hovered_panel);
	
	UI_Box* root_box = UI_BOX();
	UI_InitRootBox(root_box, (float)s->window_size.x, (float)s->window_size.y, 0);
	UI_PushBox(root_box);

	UI_PopBox(root_box);
	UI_BoxComputeRects(root_box, vec2{0, 0});
	UI_DrawBox(root_box);

	UpdatePlugins(s);
	
	// Add top bar after panel tree to make the top bar dropdowns "in front" of the root
	AddTopBar(s);

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

struct HT_OS_Events {
	HT_InputFrame* frame;
	DS_DynArray(HT_InputEvent) events;
};

static void HT_OS_BeginEvents(HT_OS_Events* s, HT_InputFrame* frame) {
	memset(s, 0, sizeof(*s));
	s->frame = frame;
	s->frame->mouse_wheel_input[0] = 0.f;
	s->frame->mouse_wheel_input[1] = 0.f;
	s->frame->raw_mouse_input[0] = 0.f;
	s->frame->raw_mouse_input[1] = 0.f;
	DS_ArrInit(&s->events, TEMP);
}

static void HT_OS_EndEvents(HT_OS_Events* s) {
	s->frame->events = s->events.data;
	s->frame->events_count = s->events.count;
}

static void HT_OS_AddEvent(HT_OS_Events* s, const OS_Event* event) {
	if (event->kind == OS_EventKind_Press) {
		HT_InputKey key = (HT_InputKey)event->key; // NOTE: OS_Key and HT_InputKey must be kept in sync!
		s->frame->key_is_down[event->key] = true;
		
		HT_InputEvent input_event = {0};
		input_event.kind = event->is_repeat ? HT_InputEventKind_Repeat : HT_InputEventKind_Press;
		input_event.key = key;
		input_event.mouse_click_index = event->mouse_click_index;
		DS_ArrPush(&s->events, input_event);
	}
	if (event->kind == OS_EventKind_Release) {
		HT_InputKey key = (HT_InputKey)event->key; // NOTE: OS_Key and HT_InputKey must be kept in sync!
		s->frame->key_is_down[event->key] = false;

		HT_InputEvent input_event = {0};
		input_event.kind = HT_InputEventKind_Release;
		input_event.key = key;
		DS_ArrPush(&s->events, input_event);
	}
	if (event->kind == OS_EventKind_TextCharacter) {
		HT_InputEvent input_event = {0};
		input_event.kind = HT_InputEventKind_TextCharacter;
		input_event.text_character = event->text_character;
		DS_ArrPush(&s->events, input_event);
	}
	if (event->kind == OS_EventKind_MouseWheel) {
		s->frame->mouse_wheel_input[1] += event->mouse_wheel;
	}
	if (event->kind == OS_EventKind_RawMouseInput) {
		s->frame->raw_mouse_input[0] += event->raw_mouse_input[0];
		s->frame->raw_mouse_input[1] += event->raw_mouse_input[1];
	}
}

int main() {

	EditorState editor_state = {};
	RenderState render_state = {};
	
	editor_state.cpu_frequency = OS_GetCPUFrequency();

	editor_state.render_state = &render_state;
	TEMP = &editor_state.temporary_arena;

	AppInit(&editor_state);

	for (;;) {
		DS_ArenaReset(&editor_state.temporary_arena);
		UI_OS_ResetFrameInputs(&editor_state.window, &editor_state.ui_inputs);

		OS_Event event;
		HT_OS_Events input_events;
		HT_OS_BeginEvents(&input_events, &editor_state.input_frame);
		while (OS_PollEvent(&editor_state.window, &event, OnResizeWindow, &editor_state)) {
			UI_OS_RegisterInputEvent(&editor_state.ui_inputs, &event);
			HT_OS_AddEvent(&input_events, &event);
		}
		HT_OS_EndEvents(&input_events);
		if (OS_WindowShouldClose(&editor_state.window)) break;

		UpdateAndDraw(&editor_state);
	}

	return 0;
}

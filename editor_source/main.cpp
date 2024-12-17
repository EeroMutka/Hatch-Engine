#include "include/ht_common.h"
#include "include/ht_editor_render.h"

#include <ht_utils/fire/fire_ui/fire_ui_backend_fire_os.h>

#ifdef HT_EDITOR_DX12
#include <ht_utils/fire/fire_ui/fire_ui_backend_dx12.h>
#endif
#ifdef HT_EDITOR_DX11
#include <ht_utils/fire/fire_ui/fire_ui_backend_dx11.h>
#endif

#include <ht_utils/fire/fire_ui/fire_ui_backend_stb_truetype.h>

// -- Globals -----------------------------

EXPORT DS_Arena* TEMP;
EXPORT DS_MemScopeNone MEM_SCOPE_NONE_;
EXPORT DS_MemScope MEM_SCOPE_TEMP_;
EXPORT uint64_t CPU_FREQUENCY;
EXPORT STR_View DEFAULT_WORKING_DIRECTORY;

extern "C" {
	EXPORT UI_State UI_STATE;
	EXPORT DS_Arena* UI_TEMP;
#ifdef HT_EDITOR_DX12
	EXPORT UI_DX12_State UI_DX12_STATE;
#endif
#ifdef HT_EDITOR_DX11
	EXPORT UI_DX11_State UI_DX11_STATE;
#endif
	EXPORT UI_STBTT_State UI_STBTT_STATE;
}

// ----------------------------------------

static STR_View QueryTabName(UI_PanelTree* tree, UI_Tab* tab) {
	return tab->name;
}

static void InitAssetTree(AssetTree* tree) {
	DS_BkArrInit(&tree->assets, DS_HEAP, 32);
	tree->root = MakeNewAsset(tree, AssetKind_Root);
	DS_MapInit(&tree->package_from_name, DS_HEAP);

	tree->name_and_type_struct_type = MakeNewAsset(tree, AssetKind_StructType);

	StructMember name_member = {0};
	StringInit(&name_member.name, "Name");
	name_member.type.kind = HT_TypeKind_String;
	DS_ArrPush(&tree->name_and_type_struct_type->struct_type.members, name_member);

	StructMember type_member = {0};
	StringInit(&type_member.name, "HT_Type");
	type_member.type.kind = HT_TypeKind_Type;
	DS_ArrPush(&tree->name_and_type_struct_type->struct_type.members, type_member);

	ComputeStructLayout(tree, tree->name_and_type_struct_type);

	tree->plugin_options_struct_type = MakeNewAsset(tree, AssetKind_StructType);

	StructMember member_data = {0};
	StringInit(&member_data.name, "data_asset");
	member_data.type.kind = HT_TypeKind_AssetRef;
	DS_ArrPush(&tree->plugin_options_struct_type->struct_type.members, member_data);

	StructMember member_source_files = {0};
	StringInit(&member_source_files.name, "code_files");
	member_source_files.type.kind = HT_TypeKind_Array;
	member_source_files.type.subkind = HT_TypeKind_AssetRef;
	DS_ArrPush(&tree->plugin_options_struct_type->struct_type.members, member_source_files);

	StructMember member_linker_inputs = {0};
	StringInit(&member_linker_inputs.name, "linker_inputs");
	member_linker_inputs.type.kind = HT_TypeKind_Array;
	member_linker_inputs.type.subkind = HT_TypeKind_AssetRef;
	DS_ArrPush(&tree->plugin_options_struct_type->struct_type.members, member_linker_inputs);


	ComputeStructLayout(tree, tree->plugin_options_struct_type);
}

static void EditorInit(DS_MemScope* persist, EditorState* s) {
	InitAssetTree(&s->asset_tree);

	s->window = OS_CreateWindow(s->window_size.x, s->window_size.y, "Hatch");
	
	RenderInit(s->render_state, s->window_size, s->window);

	UI_Init(DS_HEAP);
#ifdef HT_EDITOR_DX12
	D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor = s->render_state->srv_heap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor = s->render_state->srv_heap->GetGPUDescriptorHandleForHeapStart();
	UI_DX12_Init(s->render_state->device, atlas_cpu_descriptor, atlas_gpu_descriptor);
	UI_STBTT_Init(UI_DX12_CreateAtlas, UI_DX12_MapAtlas);
#endif
#ifdef HT_EDITOR_DX11
	UI_DX11_Init(s->render_state->device, s->render_state->dc);
	UI_STBTT_Init(UI_DX11_CreateAtlas, UI_DX11_MapAtlas);
#endif

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR_View roboto_mono_ttf, icons_ttf;
	
	OS_ReadEntireFile(persist, HATCH_DIR "/plugin_include/ht_utils/fire/fire_ui/resources/roboto_mono.ttf", &roboto_mono_ttf);
	OS_ReadEntireFile(persist, HATCH_DIR "/plugin_include/ht_utils/fire/fire_ui/resources/fontello/font/fontello.ttf", &icons_ttf);

	s->default_font = { UI_STBTT_FontInit(roboto_mono_ttf.data, -4.f), 18 };
	s->icons_font = { UI_STBTT_FontInit(icons_ttf.data, -2.f), 18 };

	// -- Hatch stuff ---------------------------------------------------------------------------

	{
		DS_ArenaInit(&s->log.arena, 4096, DS_HEAP);
		DS_ArrInit(&s->log.messages, &s->log.arena);
	}

	{
		DS_ArrInit(&s->error_list.errors, DS_HEAP);
	}

	UI_PanelTreeInit(&s->panel_tree, DS_HEAP);
	s->panel_tree.query_tab_name = QueryTabName;
	s->panel_tree.update_and_draw_tab = UpdateAndDrawTab;
	s->panel_tree.user_data = s;
	
	DS_BkArrInit(&s->plugin_instances, DS_HEAP, 32);

	DS_BkArrInit(&s->tab_classes, persist->arena, 16);
	s->assets_tab_class = CreateTabClass(s, "Assets");
	s->log_tab_class = CreateTabClass(s, "Log");
	s->errors_tab_class = CreateTabClass(s, "Errors");
	s->properties_tab_class = CreateTabClass(s, "Properties");
	s->asset_viewer_tab_class = CreateTabClass(s, "Asset Viewer");

	DS_ArrInit(&s->type_table, DS_HEAP);

	s->panel_tree.root = NewUIPanel(&s->panel_tree);
	
	InitAPI(s);
}

static void UpdateAndDraw(EditorState* s) {
	if (s->pending_start_simulation) {
		s->pending_start_simulation = false;
		s->is_simulating = true;

		for (Asset* asset = s->asset_tree.root->first_child; asset; asset = asset->next) {
			if (asset->kind != AssetKind_Package) continue;
			SavePackageToDisk(&s->asset_tree, asset);
		}
	}
	if (s->pending_stop_simulation) {
		s->pending_stop_simulation = false;
		s->is_simulating = false;

		DS_DynArray(Asset*) packages = {TEMP};
		for (Asset* asset = s->asset_tree.root->first_child; asset; asset = asset->next) {
			if (asset->kind != AssetKind_Package) continue;
			DS_ArrPush(&packages, asset);
		}
		ReloadPackages(&s->asset_tree, packages, true);
	}

	UI_BeginFrame(&s->ui_inputs, s->default_font, s->icons_font);

	s->frame = {};
	DS_ArrInit(&s->frame.queued_custom_tab_updates, TEMP);
	// DS_ArrInit(&s->frame.queued_asset_viewer_tab_updates, TEMP);
	
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
	
	RenderEndFrame(s, s->render_state);
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

int main(int argc, char** argv) {
	DS_Arena temp_arena;
	DS_ArenaInit(&temp_arena, 4096, DS_HEAP);

	TEMP = &temp_arena;
	MEM_SCOPE_NONE_ = {TEMP};
	MEM_SCOPE_TEMP_ = {TEMP, TEMP};
	CPU_FREQUENCY = OS_GetCPUFrequency();

#ifdef HT_GEN
	// The `hatch-gen` command regenerates the visual studio solution for the project in the
	// current working directory, as well as regenerates any auto-generated header files.

	STR_View cwd;
	OS_GetWorkingDir(MEM_SCOPE_TEMP, &cwd);

	AssetTree tree = {};
	InitAssetTree(&tree);
	LoadProject(&tree, cwd);

	// TODO: generate plugin headers locally into memory, then write to file if its different. This will make VS detect changes incrementally.
	//Asset* plugin_asset = ctx.queue_recompile_plugins[i];
	//RegeneratePluginHeader(tree, plugin_asset);

	GeneratePremakeAndVSProjects(&tree, cwd);
#else
	DS_Arena persistent_arena;
	DS_ArenaInit(&persistent_arena, 4096, DS_HEAP);
	DS_MemScope persist = MEM_SCOPE(&persistent_arena);

	STR_View exe_path;
	OS_GetThisExecutablePath(&persist, &exe_path);
	STR_View exe_dir = STR_BeforeLast(exe_path, '/');
	STR_View project_dir = STR_BeforeLast(exe_dir, '/');
	
	DEFAULT_WORKING_DIRECTORY = exe_dir;
	OS_SetWorkingDir(MEM_SCOPE_NONE, DEFAULT_WORKING_DIRECTORY);

	RenderState render_state = {};
	EditorState editor_state = {};
	editor_state.window_size = { 1920, 1080 };
	editor_state.render_state = &render_state;

	EditorInit(&persist, &editor_state);

	editor_state.project_directory = project_dir;
	LoadProjectIncludingEditorLayout(&editor_state, project_dir);

	for (;;) {
		DS_ArenaReset(TEMP);
		UI_OS_ResetFrameInputs(&editor_state.window, &editor_state.ui_inputs);

		OS_Event event;
		HT_OS_Events input_events;
		HT_OS_BeginEvents(&input_events, &editor_state.input_frame);
		while (OS_PollEvent(&editor_state.window, &event, OnResizeWindow, &editor_state)) {
			UI_OS_RegisterInputEvent(&editor_state.ui_inputs, &event);
			HT_OS_AddEvent(&input_events, &event);
		}
		HT_OS_EndEvents(&input_events);

		float mouse_x, mouse_y;
		OS_GetMousePosition(&editor_state.window, &mouse_x, &mouse_y);
		editor_state.input_frame.mouse_position = vec2{mouse_x, mouse_y};

		if (OS_WindowShouldClose(&editor_state.window)) break;

		UpdateAndDraw(&editor_state);
	}
#endif

	return 0;
}

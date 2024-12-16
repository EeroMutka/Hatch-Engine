
#ifdef HT_EDITOR_DX12
// Currently with the way this is structured, we have to include d3d12 here to fill the HT_INCLUDE_D3D12_API fields of the hatch API struct.
// I think this could be cleaned up!
#define HT_INCLUDE_D3D12_API
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#endif

#ifdef HT_EDITOR_DX11
#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#include <d3dcompiler.h>
#endif

#include "include/ht_common.h"
#include "include/ht_editor_render.h" // this should also be cleaned up!

#include <stdio.h> // temporarily here just for HT_DebugPrint

#ifndef HT_DYNAMIC
extern "C" HT_StaticExports HT_STATIC_EXPORTS__HT_RESERVED_DUMMY HT_ALL_STATIC_EXPORTS;
static const HT_StaticExports STATIC_EXPORTS[] = {{0} HT_ALL_STATIC_EXPORTS};
#endif

struct PluginCallContext {
	EditorState* s;
	PluginInstance* plugin;
};

// -- GLOBALS ------------------------------------------------------

// Only valid while calling a plugin DLL function
static PluginCallContext* g_plugin_call_ctx;

// -----------------------------------------------------------------

static void AssetTreeValueUI(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	EditorState* s = (EditorState*)tree->user_data;

	UI_Key key = node->key;
	Asset* asset = GetAsset(&s->asset_tree, (HT_Asset)node->key);

	UIAddAssetIcon(UI_KBOX(key), asset, s->icons_font);

	bool* is_text_editing;
	UI_BoxGetRetainedVar(UI_KBOX(key), UI_KKEY(key), &is_text_editing);

	UI_Box* text_box = UI_KBOX(key);
	if (asset->kind != AssetKind_Package && UI_DoubleClicked(text_box)) {
		*is_text_editing = true;
	}

	if (*is_text_editing) {
		UI_Text asset_name = StringToUIText(asset->name);

		UI_ValTextState* val_text_state = UI_AddValText(text_box, UI_SizeFlex(1.f), UI_SizeFit(), &asset_name, NULL);

		asset->name = UITextToString(asset_name);

		text_box->flags &= ~UI_BoxFlag_DrawBorder;
		if (!val_text_state->is_editing) {
			*is_text_editing = false;
		}
	}
	else {
		STR_View name = asset->name;
		if (asset->kind == AssetKind_Package) {
			name = asset->package.filesys_path.size > 0 ? STR_AfterLast(asset->package.filesys_path, '/') : "* Untitled Package";
		}
		
		UI_AddLabel(text_box, UI_SizeFlex(1.f), UI_SizeFit(), 0, name);
	}
}

static UI_DataTreeNode* AddAssetUITreeNode(UI_DataTreeNode* parent, Asset* asset) {
	UI_DataTreeNode* node = DS_New(UI_DataTreeNode, UI_TEMP);
	node->key = (UI_Key)asset->handle;
	node->is_open_ptr = &asset->ui_state_is_open;
	node->allow_selection = true;

	if (parent) {
		if (parent->last_child) parent->last_child->next = node;
		else parent->first_child = node;
		node->prev = parent->last_child;
		parent->last_child = node;
		node->parent = parent;
	}
	
	for (Asset* child = asset->first_child; child; child = child->next) {
		AddAssetUITreeNode(node, child);
	}
	return node;
}

EXPORT void AddTopBar(EditorState* s) {
	//UI_Box* root_box = UI_BOX();
	//UI_InitRootBox(root_box, (float)s->window_size.x, (float)s->window_size.y, 0);
	//UI_PushBox(root_box);
	//
	//UI_PopBox(root_box);
	
	UI_Box* top_bar_box = UI_BOX();
	UI_InitRootBox(top_bar_box, (float)s->window_size.x, TOP_BAR_HEIGHT, UI_BoxFlag_Horizontal|UI_BoxFlag_DrawTransparentBackground);
	UIRegisterOrderedRoot(&s->dropdown_state, top_bar_box);
	UI_PushBox(top_bar_box);

	UI_Box* file_button = UI_BOX();
	UIAddTopBarButton(file_button, UI_SizeFit(), UI_SizeFit(), "File");
	if (UI_Pressed(file_button)) {
		s->frame.file_dropdown_open = true;
	}

	//UI_Box* edit_button = UI_BOX();
	//UIAddTopBarButton(edit_button, UI_SizeFit(), UI_SizeFit(), "Edit");
	//if (UI_Pressed(edit_button)) {
	//	s->frame.edit_dropdown_open = true;
	//}
	
	UI_Vec2 dropdown_padding = {15.f, 2.f};

	s->file_dropdown_open =
		(s->file_dropdown_open && !(UI_InputWasPressed(UI_Input_MouseLeft) && s->dropdown_state.has_added_deepest_hovered_root)) ||
		(!s->file_dropdown_open && UI_Pressed(file_button));

	if (s->file_dropdown_open) {
		s->frame.file_dropdown = UI_BOX();
		s->frame.file_dropdown_button = file_button;
		UI_InitRootBox(s->frame.file_dropdown, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.file_dropdown);
		UI_PushBox(s->frame.file_dropdown);
		s->frame.file_dropdown->inner_padding = dropdown_padding;

		UI_AddLabel(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Open Project");
		
		UI_Box* gen_premake = UI_BOX();
		UI_AddLabel(gen_premake, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Generate Project (Premake5/VS2022)");

		if (UI_Clicked(gen_premake)) {
			GeneratePremakeAndVSProjects(&s->asset_tree, s->project_directory);
			s->file_dropdown_open = false;
		}

		UI_PopBox(s->frame.file_dropdown);
	}

	UI_Box* window_button = UI_BOX();
	UIAddTopBarButton(window_button, UI_SizeFit(), UI_SizeFit(), "Window");

	s->window_dropdown_open =
		(s->window_dropdown_open && !(UI_InputWasPressed(UI_Input_MouseLeft) && s->dropdown_state.has_added_deepest_hovered_root)) ||
		(!s->window_dropdown_open && UI_Pressed(window_button));

	if (s->window_dropdown_open) {
		s->frame.window_dropdown = UI_BOX();
		s->frame.window_dropdown_button = window_button;
		UI_InitRootBox(s->frame.window_dropdown, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.window_dropdown);
		UI_PushBox(s->frame.window_dropdown);
		s->frame.window_dropdown->inner_padding = dropdown_padding;
		
		int i = 0;
		for (DS_BkArrEach(&s->tab_classes, tab_i)) {
			UI_Tab* tab = DS_BkArrGet(&s->tab_classes, tab_i);
			if (tab->name.size == 0) continue; // free slot

			UI_Box* button = UI_KBOX(UI_HashInt(UI_KEY(), i));
			UI_AddLabel(button, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, tab->name);
			if (UI_Clicked(button)) {
				AddNewTabToActivePanel(&s->panel_tree, tab);
				s->window_dropdown_open = false;
			}
			i++;
		}

		UI_PopBox(s->frame.window_dropdown);
	}

	if (s->is_simulating) {
		UI_Box* stop_button = UI_BOX();
		UI_AddButton(stop_button, UI_SizeFit(), UI_SizeFit(), 0, "Stop");
		stop_button->draw_opts = DS_New(UI_BoxDrawOptArgs, TEMP);
		stop_button->draw_opts->transparent_bg_color = DS_Dup(TEMP, UI_RED);

		if (UI_Clicked(stop_button)) {
			s->pending_stop_simulation = true;
		}
	}
	else {
		UI_Box* simulate_button = UI_BOX();
		UI_AddButton(simulate_button, UI_SizeFit(), UI_SizeFit(), 0, "Simulate");
		simulate_button->draw_opts = DS_New(UI_BoxDrawOptArgs, TEMP);
		simulate_button->draw_opts->transparent_bg_color = DS_Dup(TEMP, UI_LIME);

		if (UI_Clicked(simulate_button)) {
			s->pending_start_simulation = true;
		}
	}

	//UIAddTopBarButton(file_button, UI_SizeFit(), UI_SizeFit(), "Stop");
	// 
	//UI_Box* help_button = UI_BOX();
	//UIAddTopBarButton(help_button, UI_SizeFit(), UI_SizeFit(), "Help");
	//if (UI_Clicked(help_button)) {
	//	//TODO();
	//}

	UI_PopBox(top_bar_box);
	UI_BoxComputeRects(top_bar_box, vec2{0, 0});
	UI_DrawBox(top_bar_box);
}

EXPORT void UpdateAndDrawAssetsBrowserTab(EditorState* s, UI_Key key, UI_Rect area) {
	
	/*if (UI_InputWasPressedOrRepeated(UI_Input_A) && UI_InputIsDown(UI_Input_Control)) {
		if (UI_InputIsDown(UI_Input_Shift)) {
			Asset* parent = g_assets_tree_ui_state.selection ? (Asset*)g_assets_tree_ui_state.selection : g_asset_tree.root;
			Asset* new_asset = MakeNewAsset(&g_asset_tree);
			MoveAssetToInside(&g_asset_tree, new_asset, parent);
			g_assets_tree_ui_state.selection = new_asset;
			
			parent->ui_state.is_open = true;
		}
		else {
			Asset* move_before = g_assets_tree_ui_state.selection ? (Asset*)g_assets_tree_ui_state.selection : g_asset_tree.root->last_child;
			Asset* new_asset = MakeNewAsset(&g_asset_tree);
			MoveAssetToBefore(&g_asset_tree, new_asset, move_before);
			g_assets_tree_ui_state.selection = new_asset;
		}
	}*/

	//if (UI_InputWasPressedOrRepeated(UI_Input_Delete)) {
	//}
	
	vec2 area_size = UI_RectSize(area);
	UI_Box* root = UI_KBOX(key);
	UI_InitRootBox(root, area_size.x, area_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root);
	UI_PushBox(root);
	
	UI_Box* scroll_area = UI_BBOX(root);
	UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 0);

	UI_DataTree assets_tree = {0};
	assets_tree.root = AddAssetUITreeNode(NULL, s->asset_tree.root);
	assets_tree.num_columns = 1; // 3
	assets_tree.icons_font = s->icons_font;
	assets_tree.allow_drag_n_drop = true;
	assets_tree.AddValueUI = AssetTreeValueUI;
	assets_tree.user_data = s;
	
	UI_AddDataTree(UI_BBOX(root), UI_SizeFlex(1.f), UI_SizeFit(), &assets_tree, &s->assets_tree_ui_state);

	UI_PopScrollArea(scroll_area);
	
	UI_PopBox(root);
	UI_BoxComputeRects(root, area.min);
	UI_DrawBox(root);
}

struct StructMemberValNode {
	UI_DataTreeNode base; // Must be the first member for downcasting
	void* data;
	HT_Type type;
	
	HT_String* name_rw; // if NULL, `name_ro` is used
	STR_View name_ro;
};

static void UIAddValAssetRef(EditorState* s, UI_Box* box, UI_Size w, UI_Size h, HT_Asset* handle) {
	STR_View asset_name = "(None)";
	UI_Color asset_name_color = UI_GRAY;
	
	if (*handle) {
		Asset* asset_val = GetAsset(&s->asset_tree, *handle);
		if (asset_val) {
			asset_name = asset_val->name;
			asset_name_color = UI_WHITE;
		} else {
			asset_name = "(Deleted Asset)";
		}
	}

	UI_AddBox(box, w, h, UI_BoxFlag_Clickable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground | UI_BoxFlag_Horizontal);

	if (s->assets_tree_ui_state.drag_n_dropping != 0) {
		box->draw_opts = DS_New(UI_BoxDrawOptArgs, TEMP);
		box->draw_opts->opaque_bg_color = DS_Dup(TEMP, UI_BLACK);

		if (UI_IsHovered(box)) {
			box->draw_opts->border_color = DS_Dup(TEMP, ACTIVE_COLOR);

			if (!UI_InputIsDown(UI_Input_MouseLeft)) {
				*handle = (HT_Asset)s->assets_tree_ui_state.drag_n_dropping;
			}
		}
	}

	UI_PushBox(box);

	Asset* asset_val = GetAsset(&s->asset_tree, *handle);
	if (asset_val) {
		UI_AddBox(UI_BBOX(box), 8.f, 0.f, 0); // padding
		UIAddAssetIcon(UI_BBOX(box), asset_val, s->icons_font);
	}

	UI_Box* label = UI_BBOX(box);
	UI_AddLabel(label, UI_SizeFlex(1.f), UI_SizeFit(), 0, asset_name);
	label->draw_opts = DS_New(UI_BoxDrawOptArgs, TEMP);
	label->draw_opts->text_color = DS_Dup(TEMP, asset_name_color);

	UI_Box* clear_button = UI_BBOX(box);
	UI_AddLabel(clear_button, UI_SizeFit(), UI_SizeFlex(1.f), UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder, "\x4a");
	clear_button->inner_padding.y += 2.f;
	clear_button->font = s->icons_font;
	clear_button->font.size -= 4;
	UI_PopBox(box);

	if (UI_Clicked(box) && asset_val) {
		s->assets_tree_ui_state.selection = (UI_Key)asset_val->handle;
	}

	if (UI_Clicked(clear_button)) {
		*handle = {};
	}
}

static void AddTypeSelectorDropdown(EditorState* s, UI_Box* dropdown_button, HT_TypeKind* type_kind, bool skip_array) {
	UI_Box* dropdown = UI_BBOX(dropdown_button);
	UI_InitRootBox(dropdown, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
	//dropdown->inner_padding = DEFAULT_UI_INNER_PADDING;
	UI_PushBox(dropdown);
	for (int i = 0; i < HT_TypeKind_COUNT; i++) {
		STR_View type_string = HT_TypeKindToString((HT_TypeKind)i);
		if (skip_array && i == HT_TypeKind_Array) continue;
		
		UI_Box* label = UI_KBOX(UI_HashInt(UI_BKEY(dropdown_button), i));
		UI_AddLabel(label, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable | UI_BoxFlag_Selectable, type_string);
		if (UI_Pressed(label)) {
			s->type_dropdown_open = 0;

			// Change type!!
			if (*type_kind != (HT_TypeKind)i) {
				*type_kind = (HT_TypeKind)i;

				//Asset* type_asset = (Asset*)tree->user_data;

				// Sync all struct assets data to the new type layout
				//int member_index = StructMemberNodeFindMemberIndex(type_node);
				//DS_ForSlotAllocatorEachSlot(Asset, &g_asset_tree.assets, IT) {
				//	if (AssetSlotIsEmpty(IT.elem)) continue;
				//	if (IT.elem->kind != AssetKind_StructData || IT.elem->struct_data.struct_type.asset != type_asset) continue;
				//	memset(&IT.elem->struct_data.members[member_index], 0, sizeof(StructDataMember));
				//}
			}
		}
	}
	UI_PopBox(dropdown);
	s->frame.type_dropdown = dropdown;
	s->frame.type_dropdown_button = dropdown_button;
}

static void UIAddValType(EditorState* s, UI_Key key, HT_Type* type) {
	UI_Box* kind_dropdown = UI_KBOX(key);
	UI_AddDropdownButton(kind_dropdown, UI_SizeFlex(1.f), UI_SizeFit(), 0, HT_TypeKindToString(type->kind), s->icons_font);
	if (UI_Pressed(kind_dropdown)) {
		s->type_dropdown_open = s->type_dropdown_open == kind_dropdown->key ? 0 : kind_dropdown->key;
	}
	
	UI_Box* subkind_dropdown = NULL;
	if (type->kind == HT_TypeKind_Array) {
		subkind_dropdown = UI_KBOX(key);
		UI_AddDropdownButton(subkind_dropdown, UI_SizeFlex(1.f), UI_SizeFit(), 0, HT_TypeKindToString(type->subkind), s->icons_font);
		if (UI_Pressed(subkind_dropdown)) {
			s->type_dropdown_open = s->type_dropdown_open == subkind_dropdown->key ? 0 : subkind_dropdown->key;
		}
	}

	HT_TypeKind leaf_kind = type->kind == HT_TypeKind_Array ? type->subkind : type->kind;
	if (leaf_kind == HT_TypeKind_Struct) {
		UIAddValAssetRef(s, UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &type->handle);
	}

	if (s->type_dropdown_open == kind_dropdown->key) {
		AddTypeSelectorDropdown(s, kind_dropdown, &type->kind, false);
	}

	if (subkind_dropdown != NULL && s->type_dropdown_open == subkind_dropdown->key) {
		AddTypeSelectorDropdown(s, subkind_dropdown, &type->subkind, true);
	}
}

struct StructMemberNode {
	UI_DataTreeNode base;
	Asset* type;
	int member_index;
};

static void UIStructMemberNodeAdd(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	EditorState* s = (EditorState*)tree->user_data;
	//Asset* selected_asset = (Asset*)g_assets_tree_ui_state.selection;

	StructMemberNode* member_node = (StructMemberNode*)node;
	StructMember* member = &member_node->type->struct_type.members[member_node->member_index];

	UI_Key key = UI_HashInt(node->key, column);

	if (column == 0) {
		UI_Box* var_holder = UI_GetOrAddBox(UI_KKEY(key), false);
		
		bool* is_text_editing;
		UI_BoxGetRetainedVar(var_holder, UI_KKEY(key), &is_text_editing);

		UI_Box* text_box = UI_KBOX(key);
		if (UI_DoubleClicked(text_box)) {
			*is_text_editing = true;
		}

		if (*is_text_editing) {
			UI_Text member_name = StringToUIText(member->name);

			UI_ValTextState* text_edit = UI_AddValText(text_box, UI_SizeFlex(1.f), UI_SizeFit(), &member_name, NULL);
			
			member->name = UITextToString(member_name);
			
			text_box->flags &= ~UI_BoxFlag_DrawBorder;
			if (!text_edit->is_editing) {
				*is_text_editing = false;
			}
		}
		else {
			UI_AddLabel(text_box, UI_SizeFlex(1.f), UI_SizeFit(), 0, member->name);
		}
	}
	else if (column == 1) {
		parent->inner_padding = {5.f, 1.f};

		UIAddValType(s, UI_KKEY(key), &member->type);
	}
}

static void UIAddStructDataNode(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	EditorState* s = (EditorState*)tree->user_data;
	StructMemberValNode* member_val = (StructMemberValNode*)node;
	UI_Key key = UI_KKEY(member_val->base.key);

	if (column == 0) {
		if (member_val->name_rw) {
			UI_Text ui_val = StringToUIText(*member_val->name_rw);
			UI_AddValText(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), &ui_val, NULL);
			*member_val->name_rw = UITextToString(ui_val);
		}
		else {
			UI_AddLabel(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0, member_val->name_ro);
		}
	}
	else {
		parent->inner_padding = {5.f, 1.f};

		switch (member_val->type.kind) {
		case HT_TypeKind_Float: {
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), (float*)member_val->data);
		}break;
		case HT_TypeKind_Vec2: TODO(); break;
		case HT_TypeKind_Vec3: {
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &((vec3*)member_val->data)->x);
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &((vec3*)member_val->data)->y);
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &((vec3*)member_val->data)->z);
		}break;
		case HT_TypeKind_Vec4: TODO(); break;
		case HT_TypeKind_IVec2: TODO(); break;
		case HT_TypeKind_IVec3: TODO(); break;
		case HT_TypeKind_IVec4: TODO(); break;
		case HT_TypeKind_Int: {
			UI_AddValInt(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), (int32_t*)member_val->data);
		}break;
		case HT_TypeKind_Bool: {
			UI_AddCheckbox(UI_KBOX(key), (bool*)member_val->data);
		}break;
		case HT_TypeKind_Struct: {}break;
		case HT_TypeKind_ItemGroup: {
			// UI_AddLabel(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0, "(ItemGroup: TODO)");
		}break;
		case HT_TypeKind_Array: {
			UI_Box* add_button = UI_KBOX(key);
			UI_AddButton(add_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "add");
			UI_Box* clear_button = UI_KBOX(key);
			UI_AddButton(clear_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "clear");
			
			i32 member_size, _;
			GetTypeSizeAndAlignment(&s->asset_tree, &member_val->type, &member_size, &_);

			if (UI_Clicked(add_button)) {
				ArrayPush((HT_Array*)member_val->data, member_size);
			}
			if (UI_Clicked(clear_button)) {
				ArrayClear((HT_Array*)member_val->data, member_size);
			}
		}break;
		case HT_TypeKind_AssetRef: {
			HT_Asset* val = (HT_Asset*)member_val->data;
			UIAddValAssetRef(s, UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), val);
		}break;
		case HT_TypeKind_String: {
			HT_String* val = (HT_String*)member_val->data;
			UI_Text ui_val = StringToUIText(*val);

			UI_AddValText(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), &ui_val, NULL);
			
			*val = UITextToString(ui_val);
		}break;
		case HT_TypeKind_Type: {
			HT_Type* val = (HT_Type*)member_val->data;
			UIAddValType(s, UI_KKEY(key), val);
		} break;
		case HT_TypeKind_Any: {
			HT_Any* val = (HT_Any*)member_val->data;
			UIAddValType(s, UI_KKEY(key), &val->type); // TODO: if editing type...
		} break;
		case HT_TypeKind_COUNT: break;
		case HT_TypeKind_INVALID: break;
		}
	}
}

static void AddDataTreeNode(UI_DataTreeNode* parent, UI_DataTreeNode* node) { // TODO: make this an utility fn?
	// Push to doubly linked list
	if (parent->last_child) parent->last_child->next = node;
	else parent->first_child = node;
	node->prev = parent->last_child;
	parent->last_child = node;
	node->parent = parent;
}

static void BuildStructMemberValNodes(EditorState* s, StructMemberValNode* parent, void* data, HT_Type* type) {
	if (type->kind == HT_TypeKind_Struct) {
		Asset* struct_asset = GetAsset(&s->asset_tree, type->handle);

		for (int i = 0; i < struct_asset->struct_type.members.count; i++) {
			StructMember* member = &struct_asset->struct_type.members[i];
			
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_TEMP);
			node->name_ro = member->name;
			node->type = member->type;
			node->data = (char*)data + member->offset;
			node->base.key = UI_HashInt(parent->base.key, i);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node, node->data, &node->type);
		}
	}
	else if (type->kind == HT_TypeKind_Any) {
		HT_Any* any = (HT_Any*)data;
		if (any->type.kind == HT_TypeKind_Struct) {
			BuildStructMemberValNodes(s, parent, any->data, &any->type);
		} else {
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_TEMP);
			node->name_ro = "data";
			node->type = any->type;
			node->data = any->data;
			node->base.key = UI_KKEY(parent->base.key);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;
			
			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node, node->data, &node->type);
		}
	}
	else if (type->kind == HT_TypeKind_Array) {
		HT_Array* array = (HT_Array*)data;
		
		HT_Type elem_type = *type;
		elem_type.kind = elem_type.subkind;
		
		i32 elem_size, elem_align;
		GetTypeSizeAndAlignment(&s->asset_tree, &elem_type, &elem_size, &elem_align);

		for (int j = 0; j < array->count; j++) {
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_TEMP);
			node->name_ro = STR_Form(UI_TEMP, "[%d]", j);
			node->type = elem_type;
			node->data = (char*)array->data + elem_size * j;
			node->base.key = UI_HashInt(parent->base.key, j);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node, node->data, &node->type);
		}
	}
	else if (type->kind == HT_TypeKind_ItemGroup) {
		HT_ItemGroup* group = (HT_ItemGroup*)data;
		
		HT_Type item_type = *type;
		item_type.kind = item_type.subkind;
		
		HT_ItemIndex i = group->first;
		while (i) {
			HT_ItemHeader* item = GetItemFromIndex(group, i);
			
			// This is for GetSelectedItemHandle, which is experimental.
			HT_ItemHandleDecoded item_handle_decoded = {};
			item_handle_decoded.index = i;
			HT_ItemHandle item_handle = *(HT_ItemHandle*)&item_handle_decoded;
			
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_TEMP);
			node->name_rw = &item->name;
			node->type = item_type;
			node->data = (char*)item + group->item_offset;
			node->base.key = (UI_Key)item_handle; // for item groups, the key encodes the item handle.     UI_HashInt(parent->base.key, i);
			node->base.allow_selection = true;

			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node, node->data, &node->type);
			
			i = item->next;
		}
	}
}

static void UIAddStructValueEditTree(EditorState* s, UI_Key key, void* data, Asset* struct_type) {
	StructMemberValNode root = {0};
	root.base.key = key;
	root.data = data;
	root.type.kind = HT_TypeKind_Struct;
	root.type.handle = struct_type->handle;
	BuildStructMemberValNodes(s, &root, data, &root.type);

	UI_DataTree members_tree = {0};
	members_tree.root = &root.base;
	members_tree.num_columns = 2;
	members_tree.icons_font = s->icons_font;
	members_tree.AddValueUI = UIAddStructDataNode;
	members_tree.user_data = s;
	UI_AddDataTree(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &members_tree, &s->properties_tree_data_ui_state);
}

EXPORT void UpdateAndDrawPropertiesTab(EditorState* s, UI_Key key, UI_Rect area) {
	vec2 area_size = UI_RectSize(area);
	UI_Box* root_box = UI_KBOX(key);
	UI_InitRootBox(root_box, area_size.x, area_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root_box);
	UI_PushBox(root_box);

	Asset* selected_asset = GetAsset(&s->asset_tree, (HT_Asset)s->assets_tree_ui_state.selection);
	if (selected_asset && selected_asset->kind == AssetKind_StructType) {
		StructMemberNode root = {0};
		root.base.key = UI_HashPtr(UI_KKEY(key), selected_asset);
		UI_DataTreeNode* p = &root.base;
		
		int members_count = selected_asset->struct_type.members.count;
		StructMemberNode* members = (StructMemberNode*)DS_ArenaPushZero(UI_TEMP, sizeof(StructMemberNode) * members_count);

		for (int i = 0; i < members_count; i++) {
			members[i].member_index = i;
			members[i].type = selected_asset;
			UI_DataTreeNode* n = &members[i].base;
			n->key = UI_HashInt(root.base.key, i);

			static const bool is_open = false;
			n->is_open_ptr = (bool*)&is_open;

			// Push to doubly linked list
			if (p->last_child) p->last_child->next = n;
			else p->first_child = n;
			n->prev = p->last_child;
			p->last_child = n;
			n->parent = p;
		}

		UI_DataTree members_tree = {0};
		members_tree.root = p;
		members_tree.num_columns = 2;
		members_tree.icons_font = s->icons_font;
		members_tree.AddValueUI = UIStructMemberNodeAdd;
		members_tree.user_data = s;

		UI_AddDataTree(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &members_tree, &s->properties_tree_type_ui_state);

		UI_Box* add_member_box = UI_KBOX(key);
		UI_AddButton(add_member_box, UI_SizeFit(), UI_SizeFit(), 0, "Add member");
		if (UI_Clicked(add_member_box)) {
			StructTypeAddMember(&s->asset_tree, selected_asset);
		}
	}
	
	if (selected_asset && selected_asset->kind == AssetKind_StructData) {
		Asset* struct_type = GetAsset(&s->asset_tree, selected_asset->struct_data.struct_type);
		if (struct_type == NULL) {
			UI_AddLabel(UI_KBOX(key), UI_SizeFit(), UI_SizeFit(), 0, "The struct type has been destroyed!");
		}
		else {
			UIAddStructValueEditTree(s, UI_KKEY(key), selected_asset->struct_data.data, struct_type);
		}
	}

	if (selected_asset && selected_asset->kind == AssetKind_Plugin) {
		UI_Box* row = UI_KBOX(key);

		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawOpaqueBackground);
		row->draw_opts = DS_New(UI_BoxDrawOptArgs, TEMP);
		row->draw_opts->opaque_bg_color = DS_Dup(TEMP, UI_COLOR{0, 0, 0, 200});

		UI_PushBox(row);
		UI_AddBox(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
		UIAddAssetIcon(UI_KBOX(key), selected_asset, s->icons_font);
		UI_AddLabel(UI_KBOX(key), UI_SizeFit(), UI_SizeFit(), 0, selected_asset->name);
		UI_AddBox(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);

		static bool editing_plugin = false;

		UI_Box* options_button = UI_KBOX(key);
		if (UI_Pressed(options_button)) {
			editing_plugin = !editing_plugin;
		}

		UI_BoxFlags options_flags = UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
		if (editing_plugin) 
			options_flags |= UI_BoxFlag_DrawOpaqueBackground;

		//UI_AddLabel(options_button, UI_SizeFit(), UI_SizeFit(), options_flags, "edit");
		//options_button->draw_args = UI_DrawBoxDefaultArgsInit();
		//options_button->draw_args->opaque_bg_color = UI_COLOR{50, 158, 90, 255};

		UI_PopBox(row);

		UIAddStructValueEditTree(s, UI_KKEY(key), &selected_asset->plugin.options, s->asset_tree.plugin_options_struct_type);

		PluginInstance* plugin_instance = GetPluginInstance(s, selected_asset->plugin.active_instance);
		if (plugin_instance) {
			UI_Box* stop_button = UI_KBOX(key);
			UI_AddButton(stop_button, UI_SizeFit(), UI_SizeFit(), 0, "Stop");
			if (UI_Clicked(stop_button)) {
				UnloadPlugin(s, selected_asset);
				selected_asset->plugin.active_by_request = false;
			}
		}
		else {
			UI_Box* run_button = UI_KBOX(key);
			UI_AddButton(run_button, UI_SizeFit(), UI_SizeFit(), 0, "Run");
			if (UI_Clicked(run_button)) {
				RunPlugin(s, selected_asset);
				selected_asset->plugin.active_by_request = true;
			}
		}
	}

	UI_PopBox(root_box);
	UI_BoxComputeRects(root_box, area.min);
	UI_DrawBox(root_box);
}

static void UpdateAndDrawRMBMenu(EditorState* s) {
#if 0
	static bool first_frame = true;
	if (first_frame) {
		first_frame = false;

		Asset* type_vec2 = MakeNewAsset(&g_asset_tree, AssetKind_StructType);
		MoveAssetToInside(&g_asset_tree, type_vec2, g_asset_tree.root);
		UI_TextSet(&type_vec2->name, "vec2");

		int _ = 0;
		/*{
		StructMemberNode* m1 = StructTypeAddMember(type_vec2);
		UI_TextSet(&m1->name, "x");
		StructMemberNode* m2 = StructTypeAddMember(type_vec2);
		UI_TextSet(&m2->name, "y");
		}*/

		/*Asset* type2 = MakeNewAsset(&g_asset_tree, AssetKind_StructType);
		{
		MoveAssetToInside(&g_asset_tree, type2, g_asset_tree.root);
		StructMemberNode* m1 = StructTypeAddMember(type2);
		m1->type.kind = HT_TypeKind_Struct;
		m1->type.handle = GetAssetHandle(type_vec2);
		UI_TextSet(&m1->name, "position");

		StructMemberNode* m2 = StructTypeAddMember(type2);

		StructMemberNode* m3 = StructTypeAddMember(type2);
		m3->type.kind = HT_TypeKind_Array;
		m3->type.subkind = HT_TypeKind_Float;
		UI_TextSet(&m3->name, "some_numbers");
		}*/

		/*{
		Asset* data_asset = MakeNewAsset(&g_asset_tree, AssetKind_StructData);
		MoveAssetToInside(&g_asset_tree, data_asset, g_asset_tree.root);
		InitStructDataAsset(data_asset, type2);
		}*/
	}
#endif

	bool has_hovered_tab = s->frame.hovered_panel && s->frame.hovered_panel->tabs.count > 0;
	UI_Tab* hovered_tab = has_hovered_tab ? s->frame.hovered_panel->tabs[s->frame.hovered_panel->active_tab] : NULL;

	if (has_hovered_tab && UI_InputWasPressed(UI_Input_MouseRight)) {
		s->rmb_menu_pos = UI_STATE.mouse_pos;
		s->rmb_menu_open = true;
		s->rmb_menu_tab_class = hovered_tab;
	}

	UI_Box* rmb_menu = UI_BOX();
	if (s->rmb_menu_open) {
		if (UIOrderedDropdownShouldClose(&s->dropdown_state, rmb_menu)) s->rmb_menu_open = false;
	}

	if (s->rmb_menu_open && s->rmb_menu_tab_class == s->assets_tab_class) {
		UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

		Asset* selected_asset = GetAsset(&s->asset_tree, (HT_Asset)s->assets_tree_ui_state.selection);
		if (selected_asset) {
			// for christmas MVP version, we can just do cut-n-paste for getting things into and outside of folders.
			UI_Box* new_folder = UI_BOX();
			UI_Box* new_file = UI_BOX();
			UI_Box* new_plugin = UI_BOX();
			UI_Box* new_struct_type = UI_BOX();

			UI_AddLabel(new_folder, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Folder");
			UI_AddLabel(new_struct_type, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Struct HT_Type");
			UI_AddLabel(new_plugin, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Plugin");
			UI_AddLabel(new_file, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New File");

			if (UI_Clicked(new_folder)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_Folder);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				selected_asset->ui_state_is_open = true;
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_file)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_File);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				selected_asset->ui_state_is_open = true;
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_plugin)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_Plugin);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_struct_type)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_StructType);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;
				s->rmb_menu_open = false;
			}

			if (selected_asset->kind == AssetKind_Package) {
				UI_Box* save_package = UI_BOX();
				UI_AddLabel(save_package, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Save Package");
				if (UI_Clicked(save_package)) {
					SavePackageToDisk(&s->asset_tree, selected_asset);
					s->rmb_menu_open = false;
				}
			}
		}
		else {
			UI_Box* new_package_box = UI_BOX();
			UI_AddLabel(new_package_box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Package");
			if (UI_Clicked(new_package_box)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_Package);
				MoveAssetToInside(&s->asset_tree, new_asset, s->asset_tree.root);
				new_asset->ui_state_is_open = true;
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;
				s->rmb_menu_open = false;
			}

			UI_Box* load_package_box = UI_BOX();
			UI_AddLabel(load_package_box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Load Package");
			if (UI_Clicked(load_package_box)) {
				STR_View load_package_path;
				if (OS_FolderPicker(MEM_SCOPE_TEMP, &load_package_path)) {
					//s->assets_tree_ui_state.selection = (UI_Key)...->handle
					LoadPackages(&s->asset_tree, { &load_package_path, 1 });
				}
				s->rmb_menu_open = false;
			}
		}

		if (selected_asset && selected_asset->kind == AssetKind_StructType) {
			STR_View text = STR_Form(UI_TEMP, "New Struct Data (%v)", selected_asset->name);

			UI_Box* new_struct_data = UI_BOX();
			UI_AddLabel(new_struct_data, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, text);
			if (UI_Clicked(new_struct_data)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_StructData);
				MoveAssetToInside(&s->asset_tree, new_asset, s->asset_tree.root);
				s->assets_tree_ui_state.selection = (UI_Key)new_asset->handle;

				InitStructDataAsset(&s->asset_tree, new_asset, selected_asset);

				s->rmb_menu_open = false;
			}
		}

		if (selected_asset) {
			UI_Box* delete_button = UI_BOX();
			UI_AddLabel(delete_button, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Delete");
			if (UI_Clicked(delete_button)) {
				Asset* new_selection =
					selected_asset->next ? selected_asset->next :
					selected_asset->prev ? selected_asset->prev :
					selected_asset->parent != s->asset_tree.root ? selected_asset->parent : NULL;

				DeleteAssetIncludingChildren(&s->asset_tree, selected_asset);
				s->assets_tree_ui_state.selection = (UI_Key)new_selection->handle; // TODO: here the selection should act as if moving up with keyboard instead
				s->rmb_menu_open = false;
			}
		}

		UIPopDropdown(rmb_menu);
		if (s->rmb_menu_open) {
			UI_BoxComputeRects(rmb_menu, s->rmb_menu_pos);
			UI_DrawBox(rmb_menu);
		}
	}
	else if (s->rmb_menu_open && s->rmb_menu_tab_class == s->properties_tab_class) {
		Asset* selected_asset = GetAsset(&s->asset_tree, (HT_Asset)s->assets_tree_ui_state.selection);
		if (selected_asset && selected_asset->kind == AssetKind_StructType) {
			UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

#if 0
			StructMemberNode* selected_member = (StructMemberNode*)g_properties_tree_type_ui_state.selection;
			if (selected_member) {
				ASSERT(selected_member->parent);

				UI_Box* delete_button = UI_BOX();
				UI_AddLabel(delete_button, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Delete");
				if (UI_Clicked(delete_button)) {

					// Sync all struct assets data to the new type layout
					TODO();
					//int member_index = StructMemberNodeFindMemberIndex(selected_member);
					//DS_ForSlotAllocatorEachSlot(Asset, &g_asset_tree.assets, IT) {
					//	if (AssetSlotIsEmpty(IT.elem)) continue;
					//	if (IT.elem->kind != AssetKind_StructData || IT.elem->struct_data.struct_type.asset != selected_asset) continue;
					//	DS_ArrRemove(&IT.elem->struct_data.members, member_index);
					//}

					// Remove from doubly linked list
					if (selected_member->prev) selected_member->prev->next = selected_member->next;
					else selected_member->parent->first_child = selected_member->next;
					if (selected_member->next) selected_member->next->prev = selected_member->prev;
					else selected_member->parent->last_child = selected_member->prev;

					StructMemberNodeDeinit(selected_member);
					DS_FreeSlot(&selected_asset->struct_type.all_nodes, selected_member);

					// Delete selected type node
					rmb_menu_open = false;
				}
			}
#endif

			UIPopDropdown(rmb_menu);
			UI_BoxComputeRects(rmb_menu, s->rmb_menu_pos);
			UI_DrawBox(rmb_menu);
		}
	}
	else if (s->rmb_menu_open && s->rmb_menu_tab_class == s->log_tab_class) {
		UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

		UI_Box* clear_log = UI_BOX();
		UI_AddLabel(clear_log, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Clear");
		
		UI_AddFmt(UI_BOX(), "(mem usage:%fMB)", (float)s->log.arena.total_mem_reserved / (1024.f*1024.f));
		
		if (UI_Clicked(clear_log)) {
			DS_ArenaReset(&s->log.arena);
			DS_ArrInit(&s->log.messages, &s->log.arena);
			s->rmb_menu_open = false;
		}

		UIPopDropdown(rmb_menu);
		if (s->rmb_menu_open) {
			UI_BoxComputeRects(rmb_menu, s->rmb_menu_pos);
			UI_DrawBox(rmb_menu);
		}
	}
	else {
		s->rmb_menu_open = false;
	}
}

EXPORT void RemoveErrorsByAsset(ErrorList* error_list, HT_Asset asset) {
	for (int i = 0; i < error_list->errors.count; i++) {
		Error* error = &error_list->errors[i];
		if (error->owner_asset == asset) {
			DS_ArrRemove(&error_list->errors, i);
			i--;
		}
	}
}

EXPORT PluginInstance* GetPluginInstance(EditorState* s, HT_PluginInstance handle) {
	DecodedHandle decoded = *(DecodedHandle*)&handle;
	if (decoded.bucket_index < s->plugin_instances.buckets_count) {
		PluginInstance* elems = s->plugin_instances.buckets[decoded.bucket_index].elems;
		PluginInstance* instance = &elems[decoded.elem_index];
		if (instance->handle == handle) {
			return instance;
		}
	}
	return NULL;
}

EXPORT void UpdateAndDrawDropdowns(EditorState* s) {
	UpdateAndDrawRMBMenu(s);

	if (s->frame.file_dropdown) {
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.file_dropdown);
		UI_BoxComputeRects(s->frame.file_dropdown, {s->frame.file_dropdown_button->computed_rect.min.x, s->frame.file_dropdown_button->computed_rect.max.y});
		UI_DrawBox(s->frame.file_dropdown);
	}

	if (s->frame.window_dropdown) {
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.window_dropdown);
		UI_BoxComputeRects(s->frame.window_dropdown, {s->frame.window_dropdown_button->computed_rect.min.x, s->frame.window_dropdown_button->computed_rect.max.y});
		UI_DrawBox(s->frame.window_dropdown);
	}

	if (s->frame.type_dropdown) {
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.type_dropdown);
		if (UIOrderedDropdownShouldClose(&s->dropdown_state, s->frame.type_dropdown)) {
			s->type_dropdown_open = 0;
		}
		else {
			s->frame.type_dropdown->size[0] = s->frame.type_dropdown_button->computed_expanded_size.x;
			UI_BoxComputeRects(s->frame.type_dropdown, {s->frame.type_dropdown_button->computed_rect.min.x, s->frame.type_dropdown_button->computed_rect.max.y});
			UI_DrawBox(s->frame.type_dropdown);
		}
	}

	// update drag n drop state
	if (s->assets_tree_ui_state.drag_n_dropping) {
		Asset* asset = GetAsset(&s->asset_tree, (HT_Asset)s->assets_tree_ui_state.drag_n_dropping);

		UI_Box* box = UI_BOX();
		UI_InitRootBox(box, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_Horizontal|UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		box->inner_padding = {18, 2};
		UI_PushBox(box);

		UIAddAssetIcon(UI_BOX(), asset, s->icons_font);
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, asset->name);

		UI_PopBox(box);
		UI_BoxComputeRects(box, UI_STATE.mouse_pos);
		UI_DrawBox(box);
	}
}

static void* HT_AllocatorProc(void* ptr, size_t size) {
	// We track all plugin allocations so that we can free them at once when the plugin is unloaded.

	PluginInstance* plugin = g_plugin_call_ctx->plugin;
	if (size == 0) {
		if (ptr) {
			// Move last allocation to the place of the allocation we want to free in the allocations array
			PluginAllocationHeader* header = (PluginAllocationHeader*)((char*)ptr - 16);
			PluginAllocationHeader* last_allocation = plugin->allocations[plugin->allocations.count - 1];
		
			last_allocation->allocation_index = header->allocation_index;
			plugin->allocations[header->allocation_index] = last_allocation;
		
			DS_ArrPop(&plugin->allocations);

			DS_MemFree(DS_HEAP, header);
		}
		return NULL;
	}
	else {
		// Having a 16 byte header is quite a lot of overhead per allocation... but for now its ok.

		PluginAllocationHeader* header;
		if (ptr) {
			char* allocation_base = (char*)ptr - 16;
			size_t old_size = ptr ? ((PluginAllocationHeader*)allocation_base)->size : 0;
			header = (PluginAllocationHeader*)DS_MemResizeAligned(DS_HEAP, allocation_base, old_size, 16 + size, 16);
		} else {
			header = (PluginAllocationHeader*)DS_MemAllocAligned(DS_HEAP, 16 + size, 16);
		}

		header->size = size;
		header->allocation_index = plugin->allocations.count;
		DS_ArrPush(&plugin->allocations, header);

		return (char*)header + 16;
	}
}

static void* HT_TempArenaPush(size_t size, size_t align) {
	return DS_ArenaPushAligned(TEMP, (int)size, (int)align);
}

static void* HT_GetPluginData_(/*AssetRef type_id*/) {
	EditorState* s = g_plugin_call_ctx->s;
	HT_Asset data = g_plugin_call_ctx->plugin->plugin_asset->plugin.options.data_asset;

	Asset* data_asset = GetAsset(&s->asset_tree, data);
	return data_asset ? data_asset->struct_data.data : NULL;
}

EXPORT UI_Tab* CreateTabClass(EditorState* s, STR_View name) {
	UI_Tab* tab;
	NEW_SLOT_PTR(&tab, &s->tab_classes, &s->first_free_tab_class, freelist_next);
	*tab = {};
	tab->name = STR_Clone(DS_HEAP, name);
	return tab;
}

EXPORT void DestroyTabClass(EditorState* s, UI_Tab* tab) {
	STR_Free(DS_HEAP, tab->name);
	tab->name = {}; // mark free slot with this
	FREE_SLOT_PTR(tab, &s->first_free_tab_class, freelist_next);
}

static HT_TabClass* HT_CreateTabClass(STR_View name) {
	UI_Tab* tab_class = CreateTabClass(g_plugin_call_ctx->s, name);
	tab_class->owner_plugin = g_plugin_call_ctx->plugin->plugin_asset->handle;
	return (HT_TabClass*)tab_class;
}

static void HT_DestroyTabClass(HT_TabClass* tab) {
	UI_Tab* tab_class = (UI_Tab*)tab;
	ASSERT(tab_class->owner_plugin == g_plugin_call_ctx->plugin->plugin_asset->handle); // a plugin may only destroy its own tab classes.
	DestroyTabClass(g_plugin_call_ctx->s, tab_class);
}

static bool HT_PollNextCustomTabUpdate(HT_CustomTabUpdate* tab_update) {
	EditorState* s = g_plugin_call_ctx->s;
	
	for (int i = 0; i < s->frame.queued_custom_tab_updates.count; i++) {
		HT_CustomTabUpdate* update = &s->frame.queued_custom_tab_updates[i];
		UI_Tab* tab = (UI_Tab*)update->tab_class;
		if (tab->owner_plugin == g_plugin_call_ctx->plugin->plugin_asset->handle) {
			// Remove from the queue
			*tab_update = *update;
			s->frame.queued_custom_tab_updates[i] = DS_ArrPop(&s->frame.queued_custom_tab_updates);
			return true;
		}
	}
	
	return false;
}

/*static bool HT_PollNextAssetViewerTabUpdate(HT_AssetViewerTabUpdate* tab_update) {
	EditorState* s = g_plugin_call_ctx->s;

	for (int i = 0; i < s->frame.queued_asset_viewer_tab_updates.count; i++)
	{
		HT_AssetViewerTabUpdate* update = &s->frame.queued_asset_viewer_tab_updates[i];
		Asset* data_asset = GetAsset(&s->asset_tree, update->data_asset);
		if (data_asset && data_asset->kind == AssetKind_StructData)
		{
			Asset* type_asset = GetAsset(&s->asset_tree, data_asset->struct_data.struct_type);
			if (type_asset && type_asset->struct_type.asset_viewer_registered_by_plugin == g_plugin_call_ctx->plugin->handle) {
				// Remove from the queue
				*tab_update = *update;
				s->frame.queued_asset_viewer_tab_updates[i] = DS_ArrPop(&s->frame.queued_asset_viewer_tab_updates);
				return true;
			}
		}
	}

	return false;
}*/

static STR_View HT_AssetGetFilepath(HT_Asset asset) {
	Asset* ptr = GetAsset(&g_plugin_call_ctx->s->asset_tree, asset);
	return ptr ? AssetGetAbsoluteFilepath(TEMP, ptr) : STR_View{};
}

static u64 HT_AssetGetModtime(HT_Asset asset) {
	Asset* ptr = GetAsset(&g_plugin_call_ctx->s->asset_tree, asset);
	return ptr ? ptr->modtime : 0;
}

static bool HT_RegisterAssetViewerForType(HT_Asset struct_type_asset, TabUpdateProc update_proc) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* asset = GetAsset(&s->asset_tree, struct_type_asset);
	if (asset && asset->kind == AssetKind_StructType) {
		PluginInstance* registered_by = GetPluginInstance(s, asset->struct_type.asset_viewer_registered_by_plugin);
		if (registered_by == NULL || registered_by == g_plugin_call_ctx->plugin) {
			asset->struct_type.asset_viewer_registered_by_plugin = g_plugin_call_ctx->plugin->handle;
			asset->struct_type.asset_viewer_update_proc = update_proc;
			return true;
		}
	}
	return false;
}

static void HT_DeregisterAssetViewerForType(HT_Asset struct_type_asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* asset = GetAsset(&s->asset_tree, struct_type_asset);
	if (asset && asset->kind == AssetKind_StructType) {
		PluginInstance* registered_by = GetPluginInstance(s, asset->struct_type.asset_viewer_registered_by_plugin);
		if (registered_by == g_plugin_call_ctx->plugin) {
			asset->struct_type.asset_viewer_registered_by_plugin = NULL;
		}
	}
}

static HT_Asset* HT_GetAllOpenAssetsOfType(HT_Asset struct_type_asset, int* out_count) {
	EditorState* s = g_plugin_call_ctx->s;
	
	HT_Asset* result = NULL;
	*out_count = 0;

	HT_Asset selected_asset = (HT_Asset)s->assets_tree_ui_state.selection;
	Asset* data_asset = GetAsset(&s->asset_tree, selected_asset);
	if (data_asset && data_asset->kind == AssetKind_StructData && data_asset->struct_data.struct_type == struct_type_asset) {
		result = DS_Clone(HT_Asset, TEMP, selected_asset);
		*out_count = 1;
	}

	return result;

	// TODO: loop through all tabs
	/*for (DS_BkArrEach(&s->panel_tree.panels, panel_i)) {
		UI_Panel* panel = DS_BkArrGet(&s->panel_tree.panels, panel_i);
		if (!panel->is_alive) continue;

		for (int tab_i = 0; tab_i < panel->tabs.count; tab_i++) {
			UI_Tab* tab = panel->tabs[tab_i];
			
			__debugbreak();
			//is_alive
		}
	}*/
	// loop through all open tabs
	//return 0;
}

EXPORT void UpdateAndDrawTab(UI_PanelTree* tree, UI_Tab* tab, UI_Key key, UI_Rect area_rect) {
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
	else if (tab == s->errors_tab_class) {
		UpdateAndDrawErrorsTab(s, key, area_rect);
	}
	else if (tab == s->asset_viewer_tab_class) {
		HT_Asset selected_asset = (HT_Asset)s->assets_tree_ui_state.selection;
		
		Asset* data_asset = GetAsset(&s->asset_tree, selected_asset);
		if (data_asset && data_asset->kind == AssetKind_StructData) {
			Asset* type_asset = GetAsset(&s->asset_tree, data_asset->struct_data.struct_type);
			PluginInstance* plugin = type_asset ? GetPluginInstance(s, type_asset->struct_type.asset_viewer_registered_by_plugin) : NULL;
			if (plugin) {
				HT_AssetViewerTabUpdate update = {};
				update.data_asset = selected_asset;
				update.rect_min = {(int)area_rect.min.x, (int)area_rect.min.y};
				update.rect_max = {(int)area_rect.max.x, (int)area_rect.max.y};
				
				UI_Rect parent_rect = UI_GetActiveScissorRect();
				UI_SetActiveScissorRect(area_rect);
				
				PluginCallContext ctx = {s, plugin};
				g_plugin_call_ctx = &ctx;
				type_asset->struct_type.asset_viewer_update_proc(s->api, &update);
				g_plugin_call_ctx = NULL;
				
				UI_SetActiveScissorRect(parent_rect);
			}
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

static HRESULT HT_D3DCompileFromFile(STR_View FileName, const D3D_SHADER_MACRO* pDefines,
	ID3DInclude* pInclude, const char* pEntrypoint, const char* pTarget, u32 Flags1,
	u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
{
	wchar_t* pFileName = OS_UTF8ToWide(MEM_SCOPE_TEMP, FileName, 1);
	return D3DCompileFromFile(pFileName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs);
}

static void HT_AddIndices(u32* indices, int count) {
	UI_AddIndices(indices, count, NULL);
}

static HT_Asset HT_AssetGetType(HT_Asset asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* ptr = GetAsset(&s->asset_tree, asset);
	if (ptr != NULL && ptr->kind == AssetKind_StructData) {
		return ptr->struct_data.struct_type;
	}
	return NULL;
}

static void* HT_AssetGetData(HT_Asset asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* ptr = GetAsset(&s->asset_tree, asset);
	if (ptr != NULL && ptr->kind == AssetKind_StructData) {
		return ptr->struct_data.data;
	}
	return NULL;
}

static HANDLE HT_D3DCreateEvent() { return CreateEventW(NULL, FALSE, FALSE, NULL); }
static void HT_D3DDestroyEvent(HANDLE event) { CloseHandle(event); }
static void HT_D3DWaitForEvent(HANDLE event) { WaitForSingleObjectEx(event, INFINITE, FALSE); }

#ifdef HT_EDITOR_DX12
static D3D12_CPU_DESCRIPTOR_HANDLE HT_D3D12_GetHatchRenderTargetView() {
	EditorState* s = g_plugin_call_ctx->s;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = s->render_state->rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += s->render_state->frame_index * s->render_state->rtv_descriptor_size;
	return rtv_handle;
}
#endif

#ifdef HT_EDITOR_DX11
static void D3D11_SetRenderProc(void (*render)(HT_API* ht)) {
	g_plugin_call_ctx->plugin->HT_D3D11_Render = render;
}

static ID3D11RenderTargetView* HT_D3D11_GetHatchRenderTargetView() {
	EditorState* s = g_plugin_call_ctx->s;
	return s->render_state->framebuffer_rtv;
}
#endif

HT_IMPORT void HT_LogInfo(const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	LogVArgs(&g_plugin_call_ctx->s->log, LogMessageKind_Info, fmt, args);
	va_end(args);
}

HT_IMPORT void HT_LogWarning(const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	LogVArgs(&g_plugin_call_ctx->s->log, LogMessageKind_Warning, fmt, args);
	va_end(args);
}

HT_IMPORT void HT_LogError(const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	LogVArgs(&g_plugin_call_ctx->s->log, LogMessageKind_Error, fmt, args);
	va_end(args);
}

static HT_ItemHandle HT_GetSelectedItemHandle() {
	EditorState* s = g_plugin_call_ctx->s;
	return (HT_ItemHandle)s->properties_tree_data_ui_state.selection;
}

static bool HT_IsSimulating() {
	EditorState* s = g_plugin_call_ctx->s;
	return s->is_simulating;
}

EXPORT void InitAPI(EditorState* s) {
	static HT_API api = {};
	*(void**)&api.AddVertices = UI_AddVertices;
	api.AddIndices = HT_AddIndices;
	//*(void**)&api.DrawText = HT_DrawText;
	api.AllocatorProc = HT_AllocatorProc;
	api.TempArenaPush = HT_TempArenaPush;
	api.GetPluginData = HT_GetPluginData_;
	api.RegisterAssetViewerForType = HT_RegisterAssetViewerForType;
	api.UnregisterAssetViewerForType = HT_DeregisterAssetViewerForType;
	api.GetAllOpenAssetsOfType = HT_GetAllOpenAssetsOfType;
	api.PollNextCustomTabUpdate = HT_PollNextCustomTabUpdate;
	api.IsSimulating = HT_IsSimulating;
	//api.PollNextAssetViewerTabUpdate = HT_PollNextAssetViewerTabUpdate;

#ifdef HT_EDITOR_DX12
	api.D3DCompile = D3DCompile;
	*(void**)&api.D3DCompileFromFile = HT_D3DCompileFromFile;
	api.D3D12SerializeRootSignature = D3D12SerializeRootSignature;
	api.D3D_device = s->render_state->device;
	api.D3D_queue = s->render_state->command_queue;
	api.D3DGetHatchRenderTargetView = HT_D3D12_GetHatchRenderTargetView;
	api.D3DCreateEvent = HT_D3DCreateEvent;
	api.D3DDestroyEvent = HT_D3DDestroyEvent;
	api.D3DWaitForEvent = HT_D3DWaitForEvent;
#endif

#ifdef HT_EDITOR_DX11
	api.D3D11_SetRenderProc = D3D11_SetRenderProc;
	api.D3D11_Compile = D3DCompile;
	*(void**)&api.D3D11_CompileFromFile = HT_D3DCompileFromFile;
	api.D3D11_device = s->render_state->device;
	api.D3D11_device_context = s->render_state->dc;
	api.D3D11_GetHatchRenderTargetView = HT_D3D11_GetHatchRenderTargetView;
#endif

	api.GetSelectedItemHandle = HT_GetSelectedItemHandle;
	*(void**)&api.AssetGetType = HT_AssetGetType;
	*(void**)&api.AssetGetData = HT_AssetGetData;
	*(void**)&api.AssetGetModtime = HT_AssetGetModtime;
	*(void**)&api.AssetGetFilepath = HT_AssetGetFilepath;
	*(void**)&api.CreateTabClass = HT_CreateTabClass;
	api.DestroyTabClass = HT_DestroyTabClass;
	api.input_frame = &s->input_frame;
	s->api = &api;
}

EXPORT void RunPlugin(EditorState* s, Asset* plugin_asset) {
	ASSERT(plugin_asset->plugin.active_instance == NULL);

	Asset* package = plugin_asset;
	for (;package->kind != AssetKind_Package; package = package->parent) {}
	bool ok = OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);
	ASSERT(ok);

#ifdef HT_DYNAMIC
	// TODO: only recompile if needs recompiling
	ok = RecompilePlugin(s, plugin_asset);
	if (!ok) return;
#endif

	// Allocate a plugin instance
	DS_BucketArrayIndex plugin_instance_i;
	NEW_SLOT(&plugin_instance_i, &s->plugin_instances, &s->first_free_plugin_instance, freelist_next);
	PluginInstance* plugin_instance = DS_BkArrGet(&s->plugin_instances, plugin_instance_i);

	DecodedHandle plugin_handle;
	plugin_handle.bucket_index = DS_BucketFromIndex(plugin_instance_i);
	plugin_handle.elem_index = DS_SlotFromIndex(plugin_instance_i);
	
	// keep the existing generation, except start from 1
	plugin_handle.generation = DecodeHandle(plugin_instance->handle).generation;
	if (plugin_handle.generation == 0) plugin_handle.generation = 1;
	
	*plugin_instance = {};
	plugin_instance->plugin_asset = plugin_asset;
	plugin_instance->handle = (HT_PluginInstance)EncodeHandle(plugin_handle);
	DS_ArrInit(&plugin_instance->allocations, DS_HEAP);

	STR_View plugin_name = plugin_asset->name;

#ifdef HT_DYNAMIC
	STR_View dll_path = STR_Form(TEMP, ".plugin_binaries\\%v.dll", plugin_name);
	OS_DLL* dll = OS_LoadDLL(MEM_SCOPE_NONE, dll_path);
	ok = dll != NULL;
	ASSERT(ok);

	plugin_instance->dll_handle = dll;
	*(void**)&plugin_instance->LoadPlugin = OS_GetProcAddress(dll, "HT_LoadPlugin");
	*(void**)&plugin_instance->UnloadPlugin = OS_GetProcAddress(dll, "HT_UnloadPlugin");
	*(void**)&plugin_instance->UpdatePlugin = OS_GetProcAddress(dll, "HT_UpdatePlugin");
#else
	for (int i = 1; i < DS_ArrayCount(STATIC_EXPORTS); i++) {
		HT_StaticExports exports = STATIC_EXPORTS[i];
		if (STR_Match(STR_ToV(exports.plugin_id), plugin_name)) {
			*(void**)&plugin_instance->LoadPlugin = exports.HT_LoadPlugin;
			*(void**)&plugin_instance->UnloadPlugin = exports.HT_UnloadPlugin;
			*(void**)&plugin_instance->UpdatePlugin = exports.HT_UpdatePlugin;
			break;
		}
	}
#endif
	
	ASSERT(plugin_instance->LoadPlugin);
	ASSERT(plugin_instance->UnloadPlugin);
	ASSERT(plugin_instance->UpdatePlugin);

	{
		PluginCallContext ctx = {s, plugin_instance};
		g_plugin_call_ctx = &ctx;
		plugin_instance->LoadPlugin(s->api);
		g_plugin_call_ctx = NULL;
	}

	plugin_asset->plugin.active_instance = plugin_instance->handle;

	OS_SetWorkingDir(MEM_SCOPE_NONE, DEFAULT_WORKING_DIRECTORY); // reset working directory
}

EXPORT void UnloadPlugin(EditorState* s, Asset* plugin_asset) {
	PluginInstance* plugin = GetPluginInstance(s, plugin_asset->plugin.active_instance);
	ASSERT(plugin != NULL);

	{
		PluginCallContext ctx = {s, plugin};
		g_plugin_call_ctx = &ctx;
		plugin->UnloadPlugin(s->api);
		g_plugin_call_ctx = NULL;
	}

#ifdef HT_DYNAMIC
	OS_UnloadDLL(plugin->dll_handle);
	
	STR_View pdb_filepath = STR_Form(TEMP, ".plugin_binaries/%v.pdb", plugin_asset->name.view);
	ForceVisualStudioToClosePDBFileHandle(pdb_filepath);
#endif

	plugin->plugin_asset = NULL;
	plugin->dll_handle = NULL;
	plugin->LoadPlugin = NULL;
	plugin->UnloadPlugin = NULL;
	plugin->UpdatePlugin = NULL;

	for (int i = 0; i < plugin->allocations.count; i++) {
		PluginAllocationHeader* allocation = plugin->allocations[i];
		DS_MemFree(DS_HEAP, allocation);
	}
	DS_ArrDeinit(&plugin->allocations);
	plugin->allocations = {};

	// increment the handle generation to invalidate any handles
	DecodedHandle plugin_handle = DecodeHandle(plugin->handle);
	plugin_handle.generation += 1;
	plugin->handle = (HT_PluginInstance)EncodeHandle(plugin_handle);
	
	DS_BucketArrayIndex plugin_instance_i = DS_EncodeBucketArrayIndex(plugin_handle.bucket_index, plugin_handle.elem_index);
	FREE_SLOT(plugin_instance_i, &s->plugin_instances, &s->first_free_plugin_instance, freelist_next);

	plugin_asset->plugin.active_instance = NULL;
}

#ifdef HT_EDITOR_DX12
EXPORT void D3D12_BuildPluginCommandLists(EditorState* s) {
	// Then populate the command list for plugin defined things
	
	for (DS_BkArrEach(&s->asset_tree.assets, asset_i)) {
		Asset* asset = DS_BkArrGet(&s->asset_tree.assets, asset_i);
		if (asset->kind != AssetKind_Plugin) continue;

		PluginInstance* plugin_instance = GetPluginInstance(s, asset->plugin.active_instance);
		if (plugin_instance) {
			void (*BuildPluginD3DCommandList)(HT_API* ht, ID3D12GraphicsCommandList* command_list);
			*(void**)&BuildPluginD3DCommandList = OS_GetProcAddress(plugin_instance->dll_handle, "HT_D3D12_BuildPluginCommandList");
			if (BuildPluginD3DCommandList) {

				PluginCallContext ctx = {s, plugin_instance};
				g_plugin_call_ctx = &ctx;
				BuildPluginD3DCommandList(s->api, s->render_state->command_list);
				g_plugin_call_ctx = NULL;
			}
		}
	}
}
#endif

#ifdef HT_EDITOR_DX11
EXPORT void D3D11_RenderPlugins(EditorState* s) {
	for (DS_BkArrEach(&s->plugin_instances, i)) {
		PluginInstance* plugin = DS_BkArrGet(&s->plugin_instances, i);
		if (plugin->plugin_asset == NULL) continue; // empty slot

		if (plugin->HT_D3D11_Render) {
			PluginCallContext ctx = {s, plugin};
			g_plugin_call_ctx = &ctx;
			plugin->HT_D3D11_Render(s->api);
			g_plugin_call_ctx = NULL;
		}
	}
}
#endif

EXPORT void UpdatePlugins(EditorState* s) {
	for (DS_BkArrEach(&s->asset_tree.assets, asset_i)) {
		Asset* asset = DS_BkArrGet(&s->asset_tree.assets, asset_i);
		if (asset->kind != AssetKind_Plugin) continue;
		
		PluginInstance* plugin_instance = GetPluginInstance(s, asset->plugin.active_instance);
		if (plugin_instance) {
			PluginCallContext ctx = {s, plugin_instance};
			g_plugin_call_ctx = &ctx;
			plugin_instance->UpdatePlugin(s->api);
			g_plugin_call_ctx = NULL;
		}
	}
}

// Currently with the way this is structured, we have to include d3d12 here to fill the HT_INCLUDE_D3D12_API fields of the hatch API struct.
// I think this could be cleaned up!
#define HT_INCLUDE_D3D12_API
#define WIN32_LEAN_AND_MEAN
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#include "include/ht_common.h"
#include "include/ht_editor_render.h" // this should also be cleaned up!

#include <stdio.h> // temporarily here just for HT_DebugPrint

struct PluginCallContext {
	EditorState* s;
	Asset* plugin;
};

// -- GLOBALS ------------------------------------------------------

// Only valid while calling a plugin DLL function
static PluginCallContext* g_plugin_call_ctx;

// -----------------------------------------------------------------

static void AssetTreeValueUI(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	EditorState* s = (EditorState*)tree->user_data;

	UI_Key key = node->key;
	Asset* asset = GetAsset(&s->asset_tree, (HT_AssetHandle)node->key);

	UIAddAssetIcon(UI_KBOX(key), asset, s->icons_font);

	bool* is_text_editing;
	UI_BoxGetRetainedVar(UI_KBOX(key), UI_KKEY(key), &is_text_editing);

	UI_Box* text_box = UI_KBOX(key);
	if (asset->kind != AssetKind_Package && UI_DoubleClicked(text_box)) {
		*is_text_editing = true;
	}

	if (*is_text_editing) {
		UI_ValTextState* val_text_state = UI_AddValText(text_box, UI_SizeFlex(1.f), UI_SizeFit(), &asset->name);
		text_box->flags &= ~UI_BoxFlag_DrawBorder;
		if (!val_text_state->is_editing) {
			*is_text_editing = false;
		}
	}
	else {
		STR_View name = UI_TextToStr(asset->name);
		if (asset->kind == AssetKind_Package) {
			name = asset->package.filesys_path.size > 0 ? STR_AfterLast(asset->package.filesys_path, '/') : "* Untitled Package";
		}
		
		UI_AddLabel(text_box, UI_SizeFlex(1.f), UI_SizeFit(), 0, name);
	}
}

static UI_DataTreeNode* AddAssetUITreeNode(UI_DataTreeNode* parent, Asset* asset) {
	UI_DataTreeNode* node = DS_New(UI_DataTreeNode, UI_FrameArena());
	node->key = (UI_Key)asset->handle;
	node->is_open_ptr = &asset->ui_state_is_open;
	
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
	
	UI_Box* window_button = UI_BOX();
	UIAddTopBarButton(window_button, UI_SizeFit(), UI_SizeFit(), "Window");
	
	//if (s->window_dropdown_open && UI_InputWasPressed(UI_Input_MouseLeft)) TODO();

	s->window_dropdown_open =
		(s->window_dropdown_open && !(UI_InputWasPressed(UI_Input_MouseLeft) && s->dropdown_state.has_added_deepest_hovered_root)) ||
		(!s->window_dropdown_open && UI_Pressed(window_button));

	if (s->window_dropdown_open) {
		s->frame.window_dropdown = UI_BOX();
		s->frame.window_dropdown_button = window_button;
		UI_InitRootBox(s->frame.window_dropdown, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.window_dropdown);
		UI_PushBox(s->frame.window_dropdown);
		
		int i = 0;
		DS_ForBucketArrayEach(UI_Tab, &s->tab_classes, IT) {
			if (IT.elem->name.size == 0) continue; // free slot

			UI_Box* button = UI_KBOX(UI_HashInt(UI_KEY(), i));
			UI_AddLabel(button, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, IT.elem->name);
			if (UI_Clicked(button)) {
				AddNewTabToActivePanel(&s->panel_tree, IT.elem);
				s->window_dropdown_open = false;
			}
			i++;
		}

		UI_PopBox(s->frame.window_dropdown);
	}

	//UI_Box* help_button = UI_BOX();
	//UIAddTopBarButton(help_button, UI_SizeFit(), UI_SizeFit(), "Help");
	//if (UI_Clicked(help_button)) {
	//	//TODO();
	//}

	UI_PopBox(top_bar_box);
	UI_BoxComputeRects(top_bar_box, vec2{0, 0});
	UI_DrawBox(top_bar_box);
}

EXPORT void UIAssetsBrowserTab(EditorState* s, UI_Key key, UI_Rect content_rect) {
	
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
	
	vec2 content_rect_size = UI_RectSize(content_rect);
	UI_Box* root = UI_KBOX(key);
	UI_InitRootBox(root, content_rect_size.x, content_rect_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root);
	UI_PushBox(root);
	
	UI_Box* scroll_area = UI_BBOX(root);
	UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 0);

	UI_DataTree assets_tree = {0};
	assets_tree.allow_selection = true;
	assets_tree.root = AddAssetUITreeNode(NULL, s->asset_tree.root);
	assets_tree.num_columns = 1; // 3
	assets_tree.icons_font = s->icons_font;
	assets_tree.allow_drag_n_drop = true;
	assets_tree.AddValueUI = AssetTreeValueUI;
	assets_tree.user_data = s;
	
	UI_AddDataTree(UI_BBOX(root), UI_SizeFlex(1.f), UI_SizeFit(), &assets_tree, &s->assets_tree_ui_state);

	UI_PopScrollArea(scroll_area);
	
	UI_PopBox(root);
	UI_BoxComputeRects(root, content_rect.min);
	UI_DrawBox(root);
}

struct StructMemberValNode {
	UI_DataTreeNode base; // Must be the first member for downcasting
	void* data;
	HT_Type type;
	STR_View name;
};

static void UIAddValAssetRef(EditorState* s, UI_Box* box, UI_Size w, UI_Size h, HT_AssetHandle* handle) {
	STR_View asset_name = "(None)";
	UI_Color asset_name_color = UI_GRAY;
	
	if (*handle) {
		Asset* asset_val = GetAsset(&s->asset_tree, *handle);
		if (asset_val) {
			asset_name = UI_TextToStr(asset_val->name);
			asset_name_color = UI_WHITE;
		} else {
			asset_name = "(Deleted Asset)";
		}
	}

	UI_AddBox(box, w, h, UI_BoxFlag_Clickable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground | UI_BoxFlag_Horizontal);

	if (s->assets_tree_ui_state.drag_n_dropping != 0) {
		box->draw_args = UI_DrawBoxDefaultArgsInit();
		box->draw_args->opaque_bg_color = UI_BLACK;

		if (UI_IsHovered(box)) {
			box->draw_args->border_color = ACTIVE_COLOR;

			if (!UI_InputIsDown(UI_Input_MouseLeft)) {
				*handle = (HT_AssetHandle)s->assets_tree_ui_state.drag_n_dropping;
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
	label->draw_args = UI_DrawBoxDefaultArgsInit();
	label->draw_args->text_color = asset_name_color;

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
			s->type_dropdown_open = UI_INVALID_KEY;

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
		s->type_dropdown_open = s->type_dropdown_open == kind_dropdown->key ? UI_INVALID_KEY : kind_dropdown->key;
	}
	
	UI_Box* subkind_dropdown = NULL;
	if (type->kind == HT_TypeKind_Array) {
		subkind_dropdown = UI_KBOX(key);
		UI_AddDropdownButton(subkind_dropdown, UI_SizeFlex(1.f), UI_SizeFit(), 0, HT_TypeKindToString(type->subkind), s->icons_font);
		if (UI_Pressed(subkind_dropdown)) {
			s->type_dropdown_open = s->type_dropdown_open == subkind_dropdown->key ? UI_INVALID_KEY : subkind_dropdown->key;
		}
	}

	HT_TypeKind leaf_kind = type->kind == HT_TypeKind_Array ? type->subkind : type->kind;
	if (leaf_kind == HT_TypeKind_Struct) {
		UIAddValAssetRef(s, UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &type->_struct);
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
			UI_ValTextState* text_edit = UI_AddValText(text_box, UI_SizeFlex(1.f), UI_SizeFit(), &member->name.text);
			text_box->flags &= ~UI_BoxFlag_DrawBorder;
			if (!text_edit->is_editing) {
				*is_text_editing = false;
			}
		}
		else {
			UI_AddLabel(text_box, UI_SizeFlex(1.f), UI_SizeFit(), 0, UI_TextToStr(member->name.text));
		}
	}
	else if (column == 1) {
		parent->inner_padding = {5.f, 1.f};

		UIAddValType(s, UI_KKEY(key), &member->type);
	}
}

static void UIStructDataNodeAdd(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	EditorState* s = (EditorState*)tree->user_data;
	StructMemberValNode* member_val = (StructMemberValNode*)node;
	UI_Key key = UI_KKEY(member_val->base.key);

	if (column == 0) {
		UI_AddLabel(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0, member_val->name);
	}
	else {
		parent->inner_padding = {5.f, 1.f};

		switch (member_val->type.kind) {
		case HT_TypeKind_Float: {
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), (float*)member_val->data);
		}break;
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
			HT_AssetHandle* val = (HT_AssetHandle*)member_val->data;
			UIAddValAssetRef(s, UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), val);
		}break;
		case HT_TypeKind_String: {
			String* val = (String*)member_val->data;
			if (val->text.text.allocator == NULL) {
				UI_TextInit(DS_HEAP, &val->text, "");
			}
			UI_AddValText(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), &val->text);
		}break;
		case HT_TypeKind_Type: {
			HT_Type* val = (HT_Type*)member_val->data;
			UIAddValType(s, UI_KKEY(key), val);
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

static void BuildStructMemberValNodes(EditorState* s, StructMemberValNode* parent) {
	if (parent->type.kind == HT_TypeKind_Struct) {
		Asset* struct_asset = GetAsset(&s->asset_tree, parent->type._struct);

		for (int i = 0; i < struct_asset->struct_type.members.count; i++) {
			StructMember* member = &struct_asset->struct_type.members[i];
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_FrameArena());

			node->name = UI_TextToStr(member->name.text);
			node->type = member->type;
			node->data = (char*)parent->data + member->offset;
			node->base.key = UI_HashInt(parent->base.key, i);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node);
		}
	}
	else if (parent->type.kind == HT_TypeKind_Array) {
		HT_Array* array = (HT_Array*)parent->data;
		
		HT_Type elem_type = parent->type;
		elem_type.kind = elem_type.subkind;
		
		i32 elem_size, elem_align;
		GetTypeSizeAndAlignment(&s->asset_tree, &elem_type, &elem_size, &elem_align);

		for (int j = 0; j < array->count; j++) {
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_FrameArena());
			node->name = STR_Form(UI_FrameArena(), "[%d]", j);
			node->type = elem_type;
			node->data = (char*)array->data + elem_size * j;
			node->base.key = UI_HashInt(parent->base.key, j);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node);
		}
	}
	else if (parent->type.kind == HT_TypeKind_ItemGroup) {
		HT_ItemGroup* group = (HT_ItemGroup*)parent->data;
		
		HT_Type item_type = parent->type;
		item_type.kind = item_type.subkind;
		
		i32 item_size, item_align;
		GetTypeSizeAndAlignment(&s->asset_tree, &item_type, &item_size, &item_align);
		
		i32 item_offset, item_full_size;
		CalculateItemOffsets(item_size, item_align, &item_offset, &item_full_size);
		
		HT_ItemIndex i = group->first;
		while (i._u32 != HT_ITEM_INDEX_INVALID) {
			HT_ItemHeader* item = GetItemFromIndex(group, i, item_full_size);
			
			StructMemberValNode* node = DS_New(StructMemberValNode, UI_FrameArena());
			node->name = {item->name.data, item->name.size};
			node->type = item_type;
			node->data = (char*)item + item_offset;
			node->base.key = UI_HashInt(parent->base.key, i._u32);
			
			bool* is_open = NULL;
			UI_BoxGetRetainedVar(UI_KBOX(node->base.key), UI_KEY(), &is_open);
			node->base.is_open_ptr = is_open;

			AddDataTreeNode(&parent->base, &node->base);
			BuildStructMemberValNodes(s, node);
			
			i = item->next;
		}
	}
}

static void UIAddStructValueEditTree(EditorState* s, UI_Key key, void* data, Asset* struct_type) {
	StructMemberValNode root = {0};
	root.base.key = key;
	root.data = data;
	root.type.kind = HT_TypeKind_Struct;
	root.type._struct = struct_type->handle;
	BuildStructMemberValNodes(s, &root);

	UI_DataTree members_tree = {0};
	members_tree.root = &root.base;
	members_tree.allow_selection = false;
	members_tree.num_columns = 2;
	members_tree.icons_font = s->icons_font;
	members_tree.AddValueUI = UIStructDataNodeAdd;
	members_tree.user_data = s;
	UI_AddDataTree(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), &members_tree, &s->properties_tree_data_ui_state);
}

EXPORT void UIPropertiesTab(EditorState* s, UI_Key key, UI_Rect content_rect) {
	vec2 content_rect_size = UI_RectSize(content_rect);
	UI_Box* root_box = UI_KBOX(key);
	UI_InitRootBox(root_box, content_rect_size.x, content_rect_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root_box);
	UI_PushBox(root_box);

	Asset* selected_asset = GetAsset(&s->asset_tree, (HT_AssetHandle)s->assets_tree_ui_state.selection);
	if (selected_asset && selected_asset->kind == AssetKind_StructType) {
		StructMemberNode root = {0};
		root.base.key = UI_HashPtr(UI_KKEY(key), selected_asset);
		UI_DataTreeNode* p = &root.base;
		
		int members_count = selected_asset->struct_type.members.count;
		StructMemberNode* members = (StructMemberNode*)DS_ArenaPushZero(UI_FrameArena(), sizeof(StructMemberNode) * members_count);

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
		members_tree.allow_selection = false;
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
		row->draw_args = UI_DrawBoxDefaultArgsInit();
		row->draw_args->opaque_bg_color = UI_COLOR{0, 0, 0, 200};

		UI_PushBox(row);
		UI_AddBox(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
		UIAddAssetIcon(UI_KBOX(key), selected_asset, s->icons_font);
		UI_AddLabel(UI_KBOX(key), UI_SizeFit(), UI_SizeFit(), 0, UI_TextToStr(selected_asset->name));
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

		UI_Box* compile_button = UI_KBOX(key);
		UI_AddButton(compile_button, UI_SizeFit(), UI_SizeFit(), 0, "Compile");
		if (UI_Clicked(compile_button)) {
			RecompilePlugin(s, selected_asset, s->hatch_install_directory);
		}
	}

	UI_PopBox(root_box);
	UI_BoxComputeRects(root_box, content_rect.min);
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
		m1->type._struct = GetAssetHandle(type_vec2);
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

	if (has_hovered_tab && hovered_tab == s->assets_tab_class && UI_InputWasPressed(UI_Input_MouseRight)) {
		s->rmb_menu_pos = UI_STATE.mouse_pos;
		s->rmb_menu_open = true;
		s->rmb_menu_tab_class = s->assets_tab_class;
	}

	UI_Box* rmb_menu = UI_BOX();
	if (s->rmb_menu_open) {
		if (UIOrderedDropdownShouldClose(&s->dropdown_state, rmb_menu)) s->rmb_menu_open = false;
	}

	if (s->rmb_menu_open && s->rmb_menu_tab_class == s->assets_tab_class) {
		UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

		Asset* selected_asset = GetAsset(&s->asset_tree, (HT_AssetHandle)s->assets_tree_ui_state.selection);
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
					SavePackageToDisk(selected_asset);
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
				if (OS_FolderPicker(MEM_SCOPE(TEMP), &load_package_path)) {
					s->assets_tree_ui_state.selection = (UI_Key)LoadPackageFromDisk(&s->asset_tree, load_package_path)->handle;
				}
				s->rmb_menu_open = false;
			}
		}

		if (selected_asset && selected_asset->kind == AssetKind_StructType) {
			STR_View text = STR_Form(UI_FrameArena(), "New Struct Data (%v)", UI_TextToStr(selected_asset->name));

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
		Asset* selected_asset = GetAsset(&s->asset_tree, (HT_AssetHandle)s->assets_tree_ui_state.selection);
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
		//UI_Key rmb_menu_key = UI_BOX();
		//rmb_menu_open = UI_DropdownShouldKeepOpen(rmb_menu_key);
		//UI_Box* box = UIPushDropdown(rmb_menu_key, UI_SizeFit(), UI_SizeFit());
		//box->flags |= UI_BoxFlag_ChildPadding;
		//
		//if (UI_Clicked(UI_AddButton(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), STR_V("Clear Log"))->key)) {
		//	g_next_log_message_index = 0;
		//	DS_ArenaReset(&g_log_arenas[0]);
		//	DS_ArenaReset(&g_log_arenas[1]);
		//	DS_ArrInitA(&g_log_messages[0], &g_log_arenas[0]);
		//	DS_ArrInitA(&g_log_messages[1], &g_log_arenas[1]);
		//	rmb_menu_open = false;
		//}
		//
		//UIPopDropdown(box);
		//UI_BoxComputeRects(box, rmb_menu_pos);
		//UI_DrawBox(box);
	}
	else {
		s->rmb_menu_open = false;
	}
}

EXPORT void UpdateAndDrawDropdowns(EditorState* s) {
	UpdateAndDrawRMBMenu(s);

	if (s->frame.window_dropdown) {
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.window_dropdown);
		UI_BoxComputeRects(s->frame.window_dropdown, {s->frame.window_dropdown_button->computed_rect.min.x, s->frame.window_dropdown_button->computed_rect.max.y});
		UI_DrawBox(s->frame.window_dropdown);
	}

	if (s->frame.type_dropdown) {
		UIRegisterOrderedRoot(&s->dropdown_state, s->frame.type_dropdown);
		if (UIOrderedDropdownShouldClose(&s->dropdown_state, s->frame.type_dropdown)) {
			s->type_dropdown_open = UI_INVALID_KEY;
		}
		else {
			s->frame.type_dropdown->size[0] = s->frame.type_dropdown_button->computed_expanded_size.x;
			UI_BoxComputeRects(s->frame.type_dropdown, {s->frame.type_dropdown_button->computed_rect.min.x, s->frame.type_dropdown_button->computed_rect.max.y});
			UI_DrawBox(s->frame.type_dropdown);
		}
	}

	// update drag n drop state
	if (s->assets_tree_ui_state.drag_n_dropping) {
		Asset* asset = GetAsset(&s->asset_tree, (HT_AssetHandle)s->assets_tree_ui_state.drag_n_dropping);

		UI_Box* box = UI_BOX();
		UI_InitRootBox(box, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_Horizontal|UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		box->inner_padding = {18, 2};
		UI_PushBox(box);

		UIAddAssetIcon(UI_BOX(), asset, s->icons_font);
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, UI_TextToStr(asset->name));

		UI_PopBox(box);
		UI_BoxComputeRects(box, UI_STATE.mouse_pos);
		UI_DrawBox(box);
	}
}

static void HT_DebugPrint(const char* str) {
	printf("DEBUG PRINT: %s\n", str);
}

static void* HT_AllocatorProc(void* ptr, size_t size) {
	// We track all plugin allocations so that we can free them at once when the plugin is unloaded.

	Asset_Plugin* plugin = &g_plugin_call_ctx->plugin->plugin;
	if (size == 0) {
		// Move last allocation to the place of the allocation we want to free in the allocations array
		PluginAllocationHeader* header = (PluginAllocationHeader*)((char*)ptr - 16);
		PluginAllocationHeader* last_allocation = plugin->allocations[plugin->allocations.count - 1];
		
		last_allocation->allocation_index = header->allocation_index;
		plugin->allocations[header->allocation_index] = last_allocation;
		
		DS_ArrPop(&plugin->allocations);

		DS_MemFree(DS_HEAP, header);
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
	HT_AssetHandle data = g_plugin_call_ctx->plugin->plugin.options.data;

	Asset* data_asset = GetAsset(&s->asset_tree, data);
	return data_asset ? data_asset->struct_data.data : NULL;
}

EXPORT UI_Tab* CreateTabClass(EditorState* s, STR_View name) {
	UI_Tab* tab;
	NEW_SLOT(&tab, &s->tab_classes, &s->first_free_tab_class, freelist_next);
	*tab = {};
	tab->name = STR_Clone(DS_HEAP, name);
	return tab;
}

EXPORT void DestroyTabClass(EditorState* s, UI_Tab* tab) {
	STR_Free(DS_HEAP, tab->name);
	tab->name = {}; // mark free slot with this
	FREE_SLOT(tab, &s->first_free_tab_class, freelist_next);
}

static HT_TabClass* HT_CreateTabClass(STR_View name) {
	UI_Tab* tab_class = CreateTabClass(g_plugin_call_ctx->s, name);
	tab_class->owner_plugin = g_plugin_call_ctx->plugin->handle;
	return (HT_TabClass*)tab_class;
}

static void HT_DestroyTabClass(HT_TabClass* tab) {
	UI_Tab* tab_class = (UI_Tab*)tab;
	ASSERT(tab_class->owner_plugin == g_plugin_call_ctx->plugin->handle); // a plugin may only destroy its own tab classes.
	DestroyTabClass(g_plugin_call_ctx->s, tab_class);
}

static bool HT_PollNextCustomTabUpdate(HT_CustomTabUpdate* tab_update) {
	EditorState* s = g_plugin_call_ctx->s;
	
	for (int i = 0; i < s->frame.queued_custom_tab_updates.count; i++) {
		HT_CustomTabUpdate* update = &s->frame.queued_custom_tab_updates[i];
		UI_Tab* tab = (UI_Tab*)update->tab_class;
		if (tab->owner_plugin == g_plugin_call_ctx->plugin->handle) {
			// Remove from the queue
			*tab_update = *update;
			s->frame.queued_custom_tab_updates[i] = DS_ArrPop(&s->frame.queued_custom_tab_updates);
			return true;
		}
	}
	
	return false;
}

static bool HT_PollNextAssetViewerTabUpdate(HT_AssetViewerTabUpdate* tab_update) {
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
}

static STR_View HT_AssetGetFilepath(HT_AssetHandle asset) {
	Asset* ptr = GetAsset(&g_plugin_call_ctx->s->asset_tree, asset);
	return ptr ? AssetGetFilepath(TEMP, ptr) : STR_View{};
}

static u64 HT_AssetGetModtime(HT_AssetHandle asset) {
	Asset* ptr = GetAsset(&g_plugin_call_ctx->s->asset_tree, asset);
	return ptr ? ptr->modtime : 0;
}

static bool HT_RegisterAssetViewerForType(HT_AssetHandle struct_type_asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* asset = GetAsset(&s->asset_tree, struct_type_asset);
	if (asset && asset->kind == AssetKind_StructType) {
		Asset* already_registered_by = GetAsset(&s->asset_tree, asset->struct_type.asset_viewer_registered_by_plugin);
		if (already_registered_by == NULL || already_registered_by == g_plugin_call_ctx->plugin) {
			asset->struct_type.asset_viewer_registered_by_plugin = g_plugin_call_ctx->plugin->handle;
			return true;
		}
	}
	return false;
}

static void HT_DeregisterAssetViewerForType(HT_AssetHandle struct_type_asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* asset = GetAsset(&s->asset_tree, struct_type_asset);
	if (asset && asset->kind == AssetKind_StructType) {
		if (struct_type_asset == asset->struct_type.asset_viewer_registered_by_plugin) {
			asset->struct_type.asset_viewer_registered_by_plugin = NULL;
		}
	}
}

static HRESULT HT_D3DCompileFromFile(STR_View FileName, const D3D_SHADER_MACRO* pDefines,
	ID3DInclude* pInclude, const char* pEntrypoint, const char* pTarget, u32 Flags1,
	u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs)
{
	wchar_t* pFileName = OS_UTF8ToWide(MEM_SCOPE(TEMP), FileName, 1);
	return D3DCompileFromFile(pFileName, pDefines, pInclude, pEntrypoint, pTarget, Flags1, Flags2, ppCode, ppErrorMsgs);
}

static void HT_AddIndices(u32* indices, int count) {
	UI_AddIndices(indices, count, NULL);
}

static HT_AssetHandle HT_AssetGetType(HT_AssetHandle asset) {
	EditorState* s = g_plugin_call_ctx->s;
	Asset* ptr = GetAsset(&s->asset_tree, asset);
	if (ptr != NULL && ptr->kind == AssetKind_StructData) {
		return ptr->struct_data.struct_type;
	}
	return NULL;
}

static void* HT_AssetGetData(HT_AssetHandle asset) {
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

static D3D12_CPU_DESCRIPTOR_HANDLE HT_D3DGetHatchRenderTargetView() {
	EditorState* s = g_plugin_call_ctx->s;
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = s->render_state->rtv_heap->GetCPUDescriptorHandleForHeapStart();
	rtv_handle.ptr += s->render_state->frame_index * s->render_state->rtv_descriptor_size;
	return rtv_handle;
}

EXPORT void InitAPI(EditorState* s) {
	static HT_API api = {};
	api.DebugPrint = HT_DebugPrint;
	*(void**)&api.AddVertices = UI_AddVertices;
	api.AddIndices = HT_AddIndices;
	//*(void**)&api.DrawText = HT_DrawText;
	api.AllocatorProc = HT_AllocatorProc;
	api.TempArenaPush = HT_TempArenaPush;
	api.GetPluginData = HT_GetPluginData_;
	api.RegisterAssetViewerForType = HT_RegisterAssetViewerForType;
	api.DeregisterAssetViewerForType = HT_DeregisterAssetViewerForType;
	api.PollNextCustomTabUpdate = HT_PollNextCustomTabUpdate;
	api.PollNextAssetViewerTabUpdate = HT_PollNextAssetViewerTabUpdate;
	api.D3DCompile = D3DCompile;
	*(void**)&api.D3DCompileFromFile = HT_D3DCompileFromFile;
	api.D3D12SerializeRootSignature = D3D12SerializeRootSignature;
	api.D3D_device = s->render_state->device;
	api.D3D_queue = s->render_state->command_queue;
	api.D3DGetHatchRenderTargetView = HT_D3DGetHatchRenderTargetView;
	api.D3DCreateEvent = HT_D3DCreateEvent;
	api.D3DDestroyEvent = HT_D3DDestroyEvent;
	api.D3DWaitForEvent = HT_D3DWaitForEvent;
	*(void**)&api.AssetGetType = HT_AssetGetType;
	*(void**)&api.AssetGetData = HT_AssetGetData;
	*(void**)&api.AssetGetModtime = HT_AssetGetModtime;
	*(void**)&api.AssetGetFilepath = HT_AssetGetFilepath;
	*(void**)&api.CreateTabClass = HT_CreateTabClass;
	api.DestroyTabClass = HT_DestroyTabClass;
	api.input_frame = &s->input_frame;
	s->api = &api;
}

EXPORT void UnloadPlugin(EditorState* s, Asset* plugin) {
	if (plugin->plugin.UnloadPlugin) {
		PluginCallContext ctx = {s, plugin};
		g_plugin_call_ctx = &ctx;
		plugin->plugin.UnloadPlugin(s->api);
		g_plugin_call_ctx = NULL;
	}
}

EXPORT void LoadPlugin(EditorState* s, Asset* plugin) {
	if (plugin->plugin.LoadPlugin) {
		PluginCallContext ctx = {s, plugin};
		g_plugin_call_ctx = &ctx;
		plugin->plugin.LoadPlugin(s->api);
		g_plugin_call_ctx = NULL;
	}
}

EXPORT void BuildPluginD3DCommandLists(EditorState* s) {
	// Then populate the command list for plugin defined things
	for (int bucket_i = 0; bucket_i < s->asset_tree.asset_buckets.count; bucket_i++) {
		for (int elem_i = 0; elem_i < ASSETS_PER_BUCKET; elem_i++) {
			Asset* asset = &s->asset_tree.asset_buckets[bucket_i]->assets[elem_i];

			if (asset->kind != AssetKind_Plugin) continue;
			if (asset->plugin.dll_handle) {
				void (*BuildPluginD3DCommandList)(HT_API* ht, ID3D12GraphicsCommandList* command_list);
				*(void**)&BuildPluginD3DCommandList = OS_GetProcAddress(asset->plugin.dll_handle, "HT_BuildPluginD3DCommandList");
				if (BuildPluginD3DCommandList) {

					PluginCallContext ctx = {s, asset};
					g_plugin_call_ctx = &ctx;
					BuildPluginD3DCommandList(s->api, s->render_state->command_list);
					g_plugin_call_ctx = NULL;
				}
			}
		}
	}
}

EXPORT void UpdatePlugins(EditorState* s) {
	for (int bucket_i = 0; bucket_i < s->asset_tree.asset_buckets.count; bucket_i++) {
		for (int elem_i = 0; elem_i < ASSETS_PER_BUCKET; elem_i++) {
			Asset* asset = &s->asset_tree.asset_buckets[bucket_i]->assets[elem_i];
			if (asset->kind != AssetKind_Plugin) continue;
			
			if (asset->plugin.dll_handle && asset->plugin.UpdatePlugin != NULL) {
				PluginCallContext ctx = {s, asset};
				g_plugin_call_ctx = &ctx;
				asset->plugin.UpdatePlugin(s->api);
				g_plugin_call_ctx = NULL;
			}
		}
	}
}
#include "include/ht_internal.h"

static void AssetTreeValueUI(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column) {
	UI_Key key = node->key;
	Asset* asset = (Asset*)node->key;

	UIAddAssetIcon(UI_KBOX(key), asset);

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
			name = asset->package_filesys_path.size > 0 ? STR_AfterLast(asset->package_filesys_path, '/') : "* Untitled Package";
		}
		
		UI_AddLabel(text_box, UI_SizeFlex(1.f), UI_SizeFit(), 0, name);
	}
}

static UI_DataTreeNode* AddAssetUITreeNode(UI_DataTreeNode* parent, Asset* asset) {
	UI_DataTreeNode* node = DS_New(UI_DataTreeNode, UI_FrameArena());
	node->key = (UI_Key)asset;
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
	UI_Box* top_bar_box = UI_BOX();
	UI_AddBox(top_bar_box, UI_SizeFlex(1.f), TOP_BAR_HEIGHT, UI_BoxFlag_Horizontal|UI_BoxFlag_DrawTransparentBackground);
	UI_PushBox(top_bar_box);

	UI_Box* file_button = UI_BOX();
	UIAddTopBarButton(file_button, UI_SizeFit(), UI_SizeFit(), "File");
	if (UI_Pressed(file_button)) {
		s->frame.file_dropdown_open = true;
	}

	UI_Box* edit_button = UI_BOX();
	UIAddTopBarButton(edit_button, UI_SizeFit(), UI_SizeFit(), "Edit");
	if (UI_Pressed(edit_button)) {
		s->frame.edit_dropdown_open = true;
	}

	UI_Box* panel_button = UI_BOX();
	UIAddTopBarButton(panel_button, UI_SizeFit(), UI_SizeFit(), "Window");
	if (UI_Pressed(panel_button)) {
		s->frame.panel_dropdown_open = true;
	}

	UI_Box* help_button = UI_BOX();
	UIAddTopBarButton(help_button, UI_SizeFit(), UI_SizeFit(), "Help");
	if (UI_Clicked(help_button)) {
		TODO();
	}

	UI_PopBox(top_bar_box);
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

	UI_DataTree assets_tree = {0};
	assets_tree.allow_selection = true;
	assets_tree.root = AddAssetUITreeNode(NULL, s->asset_tree.root);
	assets_tree.num_columns = 1; // 3
	assets_tree.allow_drag_n_drop = true;
	assets_tree.AddValueUI = AssetTreeValueUI;
	
	UI_AddDataTree(UI_BBOX(root), UI_SizeFlex(1.f), UI_SizeFit(), &assets_tree, &s->assets_tree_ui_state);

	UI_PopBox(root);
	UI_BoxComputeRects(root, content_rect.min);
	UI_DrawBox(root);
}

struct StructMemberValNode {
	UI_DataTreeNode base; // Must be the first member for downcasting
	void* data;
	Type type;
	STR_View name;
};

static void UIAddValAssetRef(EditorState* s, UI_Box* box, UI_Size w, UI_Size h, AssetRef* handle) {
	STR_View asset_name = "(None)";
	UI_Color asset_name_color = UI_GRAY;
	if (handle->asset) {
		if (AssetIsValid(*handle)) {
			asset_name = UI_TextToStr(handle->asset->name);
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

			Asset* asset = (Asset*)s->assets_tree_ui_state.drag_n_dropping;
			if (!UI_InputIsDown(UI_Input_MouseLeft)) {
				*handle = GetAssetHandle(asset);
			}
		}
	}

	UI_PushBox(box);

	if (AssetIsValid(*handle)) {
		UI_AddBox(UI_BBOX(box), 8.f, 0.f, 0); // padding
		UIAddAssetIcon(UI_BBOX(box), handle->asset);
	}

	UI_Box* label = UI_BBOX(box);
	UI_AddLabel(label, UI_SizeFlex(1.f), UI_SizeFit(), 0, asset_name);
	label->draw_args = UI_DrawBoxDefaultArgsInit();
	label->draw_args->text_color = asset_name_color;

	UI_Box* clear_button = UI_BBOX(box);
	UI_AddLabel(clear_button, UI_SizeFit(), UI_SizeFlex(1.f), UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder, "\x4a");
	clear_button->inner_padding.y += 2.f;
	clear_button->font = UI_STATE.icons_font;
	clear_button->font.size -= 4;
	UI_PopBox(box);

	if (UI_Clicked(box) && AssetIsValid(*handle)) {
		s->assets_tree_ui_state.selection = (UI_Key)handle->asset;
	}

	if (UI_Clicked(clear_button)) {
		*handle = {};
	}
}

static void AddTypeSelectorDropdown(EditorState* s, UI_Box* dropdown_button, TypeKind* type_kind, bool skip_array) {
	UI_Box* dropdown = UI_BBOX(dropdown_button);
	UI_InitRootBox(dropdown, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
	//dropdown->inner_padding = DEFAULT_UI_INNER_PADDING;
	UI_PushBox(dropdown);
	for (int i = 0; i < TypeKind_COUNT; i++) {
		STR_View type_string = TypeKindToString((TypeKind)i);
		if (skip_array && i == TypeKind_Array) continue;
		
		UI_Box* label = UI_KBOX(UI_HashInt(UI_BKEY(dropdown_button), i));
		UI_AddLabel(label, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable | UI_BoxFlag_Selectable, type_string);
		if (UI_Pressed(label)) {
			s->type_dropdown_open = UI_INVALID_KEY;

			// Change type!!
			if (*type_kind != (TypeKind)i) {
				*type_kind = (TypeKind)i;

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

static void UIAddValType(EditorState* s, UI_Key key, Type* type) {
	UI_Box* kind_dropdown = UI_KBOX(key);
	UI_AddDropdownButton(kind_dropdown, UI_SizeFlex(1.f), UI_SizeFit(), 0, TypeKindToString(type->kind));
	if (UI_Pressed(kind_dropdown)) {
		s->type_dropdown_open = s->type_dropdown_open == kind_dropdown->key ? UI_INVALID_KEY : kind_dropdown->key;
	}
	
	UI_Box* subkind_dropdown = NULL;
	if (type->kind == TypeKind_Array) {
		subkind_dropdown = UI_KBOX(key);
		UI_AddDropdownButton(subkind_dropdown, UI_SizeFlex(1.f), UI_SizeFit(), 0, TypeKindToString(type->subkind));
		if (UI_Pressed(subkind_dropdown)) {
			s->type_dropdown_open = s->type_dropdown_open == subkind_dropdown->key ? UI_INVALID_KEY : subkind_dropdown->key;
		}
	}

	TypeKind leaf_kind = type->kind == TypeKind_Array ? type->subkind : type->kind;
	if (leaf_kind == TypeKind_Struct) {
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
		case TypeKind_Float: {
			UI_AddValFloat(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), (float*)member_val->data);
		}break;
		case TypeKind_Int: {
			UI_AddValInt(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), (int32_t*)member_val->data);
		}break;
		case TypeKind_Bool: {
			UI_AddCheckbox(UI_KBOX(key), (bool*)member_val->data);
		}break;
		case TypeKind_Struct: {}break;
		case TypeKind_Array: {
			UI_Box* add_button = UI_KBOX(key);
			UI_AddButton(add_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "add");
			UI_Box* clear_button = UI_KBOX(key);
			UI_AddButton(clear_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "clear");
			
			i32 member_size, _;
			GetTypeSizeAndAlignment(&member_val->type, &member_size, &_);

			if (UI_Clicked(add_button)) {
				ArrayPush((Array*)member_val->data, member_size);
			}
			if (UI_Clicked(clear_button)) {
				ArrayClear((Array*)member_val->data, member_size);
			}
		}break;
		case TypeKind_AssetRef: {
			AssetRef* val = (AssetRef*)member_val->data;
			UIAddValAssetRef(s, UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), val);
		}break;
		case TypeKind_String: {
			String* val = (String*)member_val->data;
			if (val->text.text.allocator == NULL) {
				UI_TextInit(DS_HEAP, &val->text, "");
			}
			UI_AddValText(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), &val->text);
		}break;
		case TypeKind_Type: {
			Type* val = (Type*)member_val->data;
			UIAddValType(s, UI_KKEY(key), val);
		} break;
		case TypeKind_COUNT: break;
		case TypeKind_INVALID: break;
		}
	}
}

static void BuildStructMemberValNodes(StructMemberValNode* parent, Asset* struct_type, void* struct_data_base);

static void AddStructMemberNode(StructMemberValNode* parent, StructMemberValNode* node, Type* type) {
	UI_DataTreeNode* p = &parent->base;
	UI_DataTreeNode* n = &node->base;
	
	// Push to doubly linked list
	if (p->last_child) p->last_child->next = n;
	else p->first_child = n;
	n->prev = p->last_child;
	p->last_child = n;
	n->parent = p;

	if (node->type.kind == TypeKind_Struct && AssetIsValid(node->type._struct)) {
		BuildStructMemberValNodes(node, node->type._struct.asset, node->data);
	}
}

static void BuildStructMemberValNodes(StructMemberValNode* parent, Asset* struct_type, void* struct_data_base) {
	assert(struct_type->kind == AssetKind_StructType);

	for (int i = 0; i < struct_type->struct_type.members.count; i++) {
		StructMember* member = &struct_type->struct_type.members[i];
		StructMemberValNode* member_val = DS_New(StructMemberValNode, UI_FrameArena());

		member_val->name = UI_TextToStr(member->name.text);
		member_val->type = member->type;
		member_val->data = (char*)struct_data_base + member->offset;
		member_val->base.key = UI_HashInt(parent->base.key, i);
		
		bool* is_open = NULL;
		UI_BoxGetRetainedVar(UI_KBOX(member_val->base.key), UI_KEY(), &is_open);
		member_val->base.is_open_ptr = is_open;

		AddStructMemberNode(parent, member_val, &member->type);

		if (member->type.kind == TypeKind_Array) {
			Array* array = (Array*)member_val->data;
			Type elem_type = member->type;
			elem_type.kind = elem_type.subkind;
			
			i32 elem_size, elem_align;
			GetTypeSizeAndAlignment(&elem_type, &elem_size, &elem_align);

			for (int j = 0; j < array->count; j++) {
				StructMemberValNode* element_val = DS_New(StructMemberValNode, UI_FrameArena());
				
				element_val->name = STR_Form(UI_FrameArena(), "[%d]", j);
				element_val->type = elem_type;
				element_val->data = (char*)array->data + elem_size * j;
				element_val->base.key = UI_HashInt(member_val->base.key, j);
				
				bool* elem_is_open = NULL;
				UI_BoxGetRetainedVar(UI_KBOX(element_val->base.key), UI_KEY(), &elem_is_open);
				element_val->base.is_open_ptr = elem_is_open;

				AddStructMemberNode(member_val, element_val, &elem_type);
			}
		}
	}
}

static void UIAddStructValueEditTree(EditorState* s, UI_Key key, void* data, Asset* struct_type) {
	StructMemberValNode root = {0};
	root.base.key = key;

	BuildStructMemberValNodes(&root, struct_type, data);

	UI_DataTree members_tree = {0};
	members_tree.root = &root.base;
	members_tree.allow_selection = false;
	members_tree.num_columns = 2;
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

	Asset* selected_asset = (Asset*)s->assets_tree_ui_state.selection;
	if (selected_asset && selected_asset->kind == AssetKind_StructType) {
		StructMemberNode root = {0};
		root.base.key = UI_HashPtr(UI_KEY(), selected_asset);
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
		if (!AssetIsValid(selected_asset->struct_data.struct_type)) {
			UI_AddLabel(UI_KBOX(key), UI_SizeFit(), UI_SizeFit(), 0, "The struct type has been destroyed!");
		}
		else {
			Asset* struct_type = selected_asset->struct_data.struct_type.asset;
			UIAddStructValueEditTree(s, UI_KEY(), selected_asset->struct_data.data, struct_type);
		}
	}

	if (selected_asset && selected_asset->kind == AssetKind_Plugin) {
		UI_Box* row = UI_KBOX(key);

		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawOpaqueBackground);
		row->draw_args = UI_DrawBoxDefaultArgsInit();
		row->draw_args->opaque_bg_color = UI_COLOR{0, 0, 0, 200};

		UI_PushBox(row);
		UI_AddBox(UI_KBOX(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
		UIAddAssetIcon(UI_KBOX(key), selected_asset);
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

		UIAddStructValueEditTree(s, UI_KEY(), &selected_asset->plugin.options, s->asset_tree.plugin_options_struct_type);

		UI_Box* compile_button = UI_BOX();
		UI_AddButton(compile_button, UI_SizeFit(), UI_SizeFit(), 0, "Compile");
		if (UI_Clicked(compile_button)) {
			RecompilePlugin(selected_asset, s->hatch_install_directory);
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
		m1->type.kind = TypeKind_Struct;
		m1->type._struct = GetAssetHandle(type_vec2);
		UI_TextSet(&m1->name, "position");

		StructMemberNode* m2 = StructTypeAddMember(type2);

		StructMemberNode* m3 = StructTypeAddMember(type2);
		m3->type.kind = TypeKind_Array;
		m3->type.subkind = TypeKind_Float;
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
	//UI_Tab** hovered_tab = NULL;
	//if () {
	//hovered_tab = &s->frame.hovered_panel->tabs[s->frame.hovered_panel->active_tab];
	//}

	if (has_hovered_tab && UI_InputWasPressed(UI_Input_MouseRight)) {
		s->rmb_menu_pos = UI_STATE.mouse_pos;
		s->rmb_menu_open = true;
		//rmb_menu_tab_kind = hovered_tab->kind;
	}

	UI_Box* rmb_menu = UI_BOX();
	if (s->rmb_menu_open) {
		if (UIOrderedDropdownShouldClose(&s->dropdown_state, rmb_menu)) s->rmb_menu_open = false;
	}

	if (s->rmb_menu_open && s->rmb_menu_tab_kind == TabKind_Assets) {
		UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

		Asset* selected_asset = (Asset*)s->assets_tree_ui_state.selection;
		if (selected_asset) {
			// for christmas MVP version, we can just do cut-n-paste for getting things into and outside of folders.
			UI_Box* new_folder = UI_BOX();
			UI_Box* new_code_file = UI_BOX();
			UI_Box* new_plugin = UI_BOX();
			UI_Box* new_struct_type = UI_BOX();

			UI_AddLabel(new_folder, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Folder");
			UI_AddLabel(new_struct_type, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Struct Type");
			UI_AddLabel(new_plugin, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New Plugin");
			UI_AddLabel(new_code_file, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "New C++ File");

			if (UI_Clicked(new_folder)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_Folder);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				selected_asset->ui_state_is_open = true;
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_code_file)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_CPP);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				selected_asset->ui_state_is_open = true;
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_plugin)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_Plugin);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;
				s->rmb_menu_open = false;
			}
			if (UI_Clicked(new_struct_type)) {
				Asset* new_asset = MakeNewAsset(&s->asset_tree, AssetKind_StructType);
				MoveAssetToInside(&s->asset_tree, new_asset, selected_asset);
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;
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
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;
				s->rmb_menu_open = false;
			}

			UI_Box* load_package_box = UI_BOX();
			UI_AddLabel(load_package_box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Clickable, "Load Package");
			if (UI_Clicked(load_package_box)) {
				STR_View load_package_path;
				if (OS_FolderPicker(MEM_SCOPE(TEMP), &load_package_path)) {
					s->assets_tree_ui_state.selection = (UI_Key)LoadPackageFromDisk(&s->asset_tree, load_package_path);
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
				s->assets_tree_ui_state.selection = (UI_Key)new_asset;

				InitStructDataAsset(new_asset, selected_asset);

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
				s->assets_tree_ui_state.selection = (UI_Key)new_selection; // TODO: here the selection should act as if moving up with keyboard instead
				s->rmb_menu_open = false;
			}
		}

		UIPopDropdown(rmb_menu);
		if (s->rmb_menu_open) {
			UI_BoxComputeRects(rmb_menu, s->rmb_menu_pos);
			UI_DrawBox(rmb_menu);
		}
	}
	else if (s->rmb_menu_open && s->rmb_menu_tab_kind == TabKind_Properties) {
		Asset* selected_asset = (Asset*)s->assets_tree_ui_state.selection;
		if (selected_asset && selected_asset->kind == AssetKind_StructType) {
			UIPushDropdown(&s->dropdown_state, rmb_menu, UI_SizeFit(), UI_SizeFit());

#if 0
			StructMemberNode* selected_member = (StructMemberNode*)g_properties_tree_type_ui_state.selection;
			if (selected_member) {
				assert(selected_member->parent);

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
	else if (s->rmb_menu_open && s->rmb_menu_tab_kind == TabKind_Log) {
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
		Asset* asset = (Asset*)s->assets_tree_ui_state.drag_n_dropping;

		UI_Box* box = UI_BOX();
		UI_InitRootBox(box, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_Horizontal|UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
		box->inner_padding = {18, 2};
		UI_PushBox(box);

		UIAddAssetIcon(UI_BOX(), asset);
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, UI_TextToStr(asset->name));

		UI_PopBox(box);
		UI_BoxComputeRects(box, UI_STATE.mouse_pos);
		UI_DrawBox(box);
	}
}

static void HT_DebugPrint(const char* str) {
	TODO();
	//printf("DEBUG PRINT: %s\n", str);
}

static void HT_DrawText(STR_View text, vec2 pos, UI_AlignH align_h, int font_size, UI_Color color) {
	UI_DrawText(text, {UI_STATE.base_font.id, (uint16_t)font_size}, pos, align_h, color, NULL);
}

static void* HT_AllocatorProc(void* ptr, size_t size, size_t align) {
	return DS_MemReallocAligned(DS_HEAP, ptr, size, align);
}

EXPORT void UpdatePlugins(EditorState* s) {
	HT_API api = {0};
	api.DebugPrint = HT_DebugPrint;
	*(void**)&api.AddVertices = UI_AddVertices;
	*(void**)&api.AddIndices = UI_AddIndices;
	*(void**)&api.DrawText = HT_DrawText;
	api.AllocatorProc = HT_AllocatorProc;

	DS_ForSlotAllocatorEachSlot(Asset, &s->asset_tree.assets, IT) {
		Asset* plugin = IT.elem;
		if (AssetSlotIsEmpty(plugin)) continue;
		if (plugin->kind != AssetKind_Plugin) continue;

		if (plugin->plugin.dll_handle) {
			plugin->plugin.dll_UpdatePlugin(&api);
		}
	}
}
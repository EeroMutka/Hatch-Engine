#include "include/ht_internal.h"

EXPORT void UIAddPadY(UI_Box* box) {
	UI_AddBox(box, UI_SizeFit(), 10.f, 0); // Y-padding
}

EXPORT void UIPushDropdown(UIDropdownState* s, UI_Box* box, UI_Size w, UI_Size h) {
	UI_InitRootBox(box, w, h, UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder);
	UIRegisterOrderedRoot(s, box);
	UI_PushBox(box);
}

EXPORT void UIPopDropdown(UI_Box* box) {
	UI_PopBox(box);
}

EXPORT void UIAddTopBarButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string) {
	UI_AddLabel(box, w, h, UI_BoxFlag_Clickable|UI_BoxFlag_Selectable, string);
}

EXPORT void UIAddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string) {
	UI_AddLabel(box, w, h, UI_BoxFlag_Clickable|UI_BoxFlag_Selectable, string);
}

EXPORT bool UIOrderedDropdownShouldClose(UIDropdownState* s, UI_Box* box) {
	// If we click anywhere in the UI that has been added during this frame, we want to close this dropdown.
	return UI_InputWasPressed(UI_Input_MouseLeft) && s->has_added_deepest_hovered_root && box->prev_frame != NULL;
}

EXPORT void UI_AddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string, UI_Font icons_font) {
	UI_ProfEnter();
	flags |= UI_BoxFlag_Horizontal | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder |UI_BoxFlag_DrawTransparentBackground;
	UI_AddBox(box, w, h, flags);
	UI_PushBox(box);

	UI_AddLabel(UI_BBOX(box), UI_SizeFlex(1.f), UI_SizeFit(), 0, string);

	UI_Box* icon_box = UI_BBOX(box);
	UI_AddLabel(icon_box, UI_SizeFit(), UI_SizeFit(), 0, STR_V("\x44"));
	icon_box->font = icons_font;

	UI_PopBox(box);
	UI_ProfExit();
}

EXPORT void UI_PanelTreeInit(UI_PanelTree* tree, DS_Allocator* allocator) {
	*tree = {};
	DS_BucketArrayInit(&tree->panels, allocator, 8);
}

EXPORT void UIDropdownStateBeginFrame(UIDropdownState* s) {
	s->deepest_hovered_root = s->deepest_hovered_root_new;
	s->deepest_hovered_root_new = UI_INVALID_KEY;
	s->has_added_deepest_hovered_root = false;
}

EXPORT void UIRegisterOrderedRoot(UIDropdownState* s, UI_Box* dropdown) {
	if (UI_IsMouseInsideOf(dropdown)) {
		s->deepest_hovered_root_new = dropdown->key;
	}
	if (s->deepest_hovered_root == dropdown->key) {
		s->has_added_deepest_hovered_root = true;
	}
	else {
		dropdown->flags |= UI_BoxFlag_NoHover;
	}
}

EXPORT UI_Panel* NewUIPanel(UI_PanelTree* tree) {
	UI_Panel* panel;
	if (tree->first_free_panel) {
		panel = tree->first_free_panel;
		tree->first_free_panel = tree->first_free_panel->freelist_next;
	}
	else {
		panel = (UI_Panel*)DS_BucketArrayPushUndef(&tree->panels);
	}
	*panel = UI_Panel{0};
	DS_ArrInit(&panel->tabs, DS_HEAP);
	return panel;
}

EXPORT void FreeUIPanel(UI_PanelTree* tree, UI_Panel* panel) {
	DS_ArrDeinit(&panel->tabs);
	panel->freelist_next = tree->first_free_panel;
	tree->first_free_panel = panel;
}

EXPORT void UI_PanelTreeUpdateAndDraw(UI_PanelTree* tree, UI_Panel* panel, UI_Rect area_rect, bool splitter_is_hovered, UI_Font icons_font, UI_Panel** out_hovered) {
	if (panel->end_child[0] == NULL) {
		// leaf panel
		
		float tab_area_height = 28.f;
		
		UI_Rect content_rect = area_rect;
		content_rect.min.y += tab_area_height;

		UI_Vec2 panel_size = UI_RectSize(area_rect);

		UI_Box* root = UI_KBOX(UI_HashPtr(UI_KEY(), panel));
		UI_InitRootBox(root, panel_size.x, panel_size.y, UI_BoxFlag_DrawBorder);
		// TODO:  //UIRegisterOrderedRoot(root);
		assert(root->parent == NULL);

		//UI_PushBox(root); // UI_PushRootBox(UI_KEY1(panel_key), area_rect.min, UI_SizePx(panel_size.x), UI_SizePx(panel_size.y), flags);

		if (!splitter_is_hovered && (UI_Pressed(root) || UI_PressedEx(root, UI_Input_MouseRight))) {
			tree->active_panel = panel;
		}

		// maybe the active tab in active panel could be highlighted with some special color instead of this border color thing.
		// And maybe instead of Window -> menu for splitting, we could do right click menu (on empty region or tab) for splitting.
		// ALSO: tab buttons should have (very?) rounded corners only at the top, not bottom.
		if (tree->active_panel == panel && panel->tabs.count == 0) {
			root->draw_args = UI_DrawBoxDefaultArgsInit();
			root->draw_args->border_color = ACTIVE_COLOR;
		}

		UI_BoxComputeRects(root, area_rect.min);
		UI_DrawBox(root);

		// tab area
		UI_Box* tab_area = UI_BBOX(root);
		UI_InitRootBox(tab_area, panel_size.x, tab_area_height, UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder);
		// TODO: //UIRegisterOrderedRoot(tab_area);
		UI_PushBox(tab_area);

		int should_close_tab = -1;

		// Add tabs
		for (int i = 0; i < panel->tabs.count; i++) {
			UI_Tab* tab = panel->tabs[i];

			UI_BoxFlags button_flags = UI_BoxFlag_Horizontal | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder;
			if (i == panel->active_tab) {
				button_flags |= UI_BoxFlag_DrawTransparentBackground;
			}

			UI_Box* tab_button = UI_KBOX(UI_HashInt(root->key, i));
			UI_AddBox(tab_button, UI_SizeFit(), UI_SizeFit(), button_flags);

			if (i == panel->active_tab && panel == tree->active_panel) {
				tab_button->draw_args = UI_DrawBoxDefaultArgsInit();
				tab_button->draw_args->transparent_bg_color = ACTIVE_COLOR;
			}

			UI_PushBox(tab_button);
			
			STR_View tab_name = tree->query_tab_name(tree, tab);
			UI_AddLabel(UI_BBOX(tab_button), UI_SizeFit(), UI_SizeFit(), 0, tab_name);

			UI_BoxFlags close_flags = UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_HasText;
			UI_Box* tab_close_button = UI_BBOX(tab_button);
			UI_AddLabel(tab_close_button, UI_SizeFit(), UI_SizeFit(), close_flags, "J");
			tab_close_button->font = icons_font;

			if (UI_Clicked(tab_close_button)) {
				should_close_tab = i;
			}

			UI_PopBox(tab_button);

			if (UI_PressedIdle(tab_button)) { // bring to view
				tree->active_panel = panel;
				panel->active_tab = i;
			}
		}

		UI_PopBox(tab_area);
		UI_BoxComputeRects(tab_area, area_rect.min);
		UI_DrawBox(tab_area);

		if (should_close_tab != -1) {
			DS_ArrRemove(&panel->tabs, should_close_tab);
			if (panel->active_tab > should_close_tab) panel->active_tab -= 1;
			panel->active_tab = UI_Min(panel->active_tab, panel->tabs.count - 1); // clamp the active tab index
		}

		if (panel->tabs.count > 0) {
			UI_Tab* tab = panel->tabs[panel->active_tab];
			tree->update_and_draw_tab(tree, tab, root->key, content_rect);
		}

		if (UI_PointIsInRect(area_rect, UI_STATE.mouse_pos)) {
			*out_hovered = panel;
		}
	}
	else {
		assert(panel->tabs.count == 0); // only leaf panels can have tabs
		assert(panel->end_child[0] != panel->end_child[1]); // a panel may not have only one child

		//DS_DynArray(float) panel_end_offsets = {UI_FrameArena()};
		//
		//float offset = 0.f;
		int panel_count = 0;
		for (UI_Panel* child = panel->end_child[0]; child; child = child->link[1]) {
			panel_count++;
			//offset += child->size;
			//DS_ArrPush(&panel_end_offsets, offset);
		}

		UI_Axis X = 1 - panel->split_along;
		UI_Key splitters_key = UI_HashPtr(UI_KEY(), panel);
		UI_SplittersState* splitters_state = UI_Splitters(splitters_key, area_rect, X, panel_count, 6.f);

		splitter_is_hovered = splitter_is_hovered || splitters_state->hovering_splitter;

		float start = 0.f;
		int i = 0;
		UI_Panel* prev = NULL;
		for (UI_Panel* child = panel->end_child[0]; child; child = child->link[1]) {
			assert(prev == child->link[0]); // tree validation
			assert(child->parent == panel); // tree validation

			// hmm... how can you click a rectangle but also splitter?
			float end = splitters_state->panel_end_offsets[i];

			//child->size = end - start; // write back the new panel size

			UI_Rect rect = area_rect;
			rect.min._[X] = area_rect.min._[X] + start + 1.f;
			rect.max._[X] = area_rect.min._[X] + end - 1.f;
			UI_PanelTreeUpdateAndDraw(tree, child, rect, splitter_is_hovered, icons_font, out_hovered);

			start = end;
			i++;
			prev = child;
		}
	}
}

// Split panel and return the split
EXPORT UI_Panel* SplitPanel(UI_PanelTree* tree, UI_Panel* panel, UI_Axis split_along) {
	assert(panel->end_child[0] == NULL); // panel must be a leaf
	UI_Panel* parent = panel->parent;
	UI_Panel* split = NULL;

	if (parent && parent->split_along == split_along) {
		// Add one more panel at the end
		split = parent;
		UI_Panel* new_panel = NewUIPanel(tree);

		new_panel->parent = split;
		if (split->end_child[0]) split->end_child[1]->link[1] = new_panel;
		else split->end_child[0] = new_panel;
		new_panel->link[0] = split->end_child[1];
		split->end_child[1] = new_panel;

		new_panel->size = panel->size;
		// tree->active_panel = new_panel;
	}
	else {
		// Split the active panel and insert itself as the first child of the split
		split = NewUIPanel(tree);
		split->size = panel->size;
		split->split_along = split_along;

		UI_Panel* new_leaf = NewUIPanel(tree);
		new_leaf->size = panel->size;

		// Replace panel with new_split
		split->parent = panel->parent;
		split->link[0] = panel->link[0];
		split->link[1] = panel->link[1];

		if (parent) {
			if (split->link[0]) split->link[0]->link[1] = split;
			else parent->end_child[0] = split;
			if (split->link[1]) split->link[1]->link[0] = split;
			else parent->end_child[1] = split;
		}
		else {
			tree->root = split;
		}

		panel->parent = split;
		new_leaf->parent = split;
		split->end_child[0] = panel;
		split->end_child[1] = new_leaf;
		panel->link[0] = NULL;
		panel->link[1] = new_leaf;
		new_leaf->link[0] = panel;
	}
	return split;
}

EXPORT void ClosePanel(UI_PanelTree* tree, UI_Panel* panel) {
	assert(panel->end_child[0] == NULL); // panel must be a leaf

	UI_Panel* parent = panel->parent;
	DS_ArrClear(&panel->tabs);
	if (parent == NULL) return;

	// Remove panel from its parents child list
	if (panel->link[0]) panel->link[0]->link[1] = panel->link[1];
	else parent->end_child[0] = panel->link[1];
	if (panel->link[1]) panel->link[1]->link[0] = panel->link[0];
	else parent->end_child[1] = panel->link[0];

	FreeUIPanel(tree, panel);

	assert(parent->end_child[0] != NULL); // child lists must always have more than 1 element

	// if the parent now has only one child, then replace the parent with the last child
	if (parent->end_child[0] && parent->end_child[0] == parent->end_child[1]) {
		UI_Panel* last_child = parent->end_child[0];

		last_child->parent = parent->parent;
		last_child->size = parent->size;
		last_child->link[0] = parent->link[0];
		last_child->link[1] = parent->link[1];

		if (parent->parent) { // if not root
			if (last_child->link[0]) last_child->link[0]->link[1] = last_child;
			else parent->parent->end_child[0] = last_child;
			if (last_child->link[1]) last_child->link[1]->link[0] = last_child;
			else parent->parent->end_child[1] = last_child;
		}
		else {
			tree->root = last_child;
		}
		FreeUIPanel(tree, parent);
	}
}

EXPORT void AddNewTabToActivePanel(UI_PanelTree* tree, UI_Tab* tab) {
	if (tree->active_panel) {
		tree->active_panel->active_tab = tree->active_panel->tabs.count;
		DS_ArrPush(&tree->active_panel->tabs, tab);
	}
}

EXPORT void UIAddAssetIcon(UI_Box* box, Asset* asset, UI_Font icons_font) {
	UI_AddLabel(box, 15.f, UI_SizeFit(), 0,
		asset->kind == AssetKind_Package ? "\x4c" :
		asset->kind == AssetKind_Folder ? (asset->ui_state_is_open ? "\x56" : "\x55") :
		asset->kind == AssetKind_StructType ? "\x4e" :
		asset->kind == AssetKind_StructData ? "\x4f" :
		asset->kind == AssetKind_Plugin ? "\x4b" :
		"\x52"
	);
	box->inner_padding.x = 2.f;
	box->inner_padding.y = 7.f;
	box->font = icons_font;
	box->font.size -= 4;
	//box->draw_args = UI_DrawBoxDefaultArgsInit();
	//box->draw_args->text_color =
	//	asset->kind == AssetKind_StructType ? UI_PURPLE :
	//	asset->kind == AssetKind_StructData ? UI_GOLD :
	//	UI_WHITE;
}

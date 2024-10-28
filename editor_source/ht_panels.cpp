#include "ht_internal.h"

static UIPanel* NewUIPanel() {
	UIPanel* panel = (UIPanel*)DS_TakeSlot(&g_panels);
	*panel = UIPanel{0};
	DS_ArrInit(&panel->tabs, DS_HEAP);
	return panel;
}

static void FreeUIPanel(UIPanel* panel) {
	for (int i = 0; i < panel->tabs.count; i++) FreeUITab(&panel->tabs.data[i]);
	DS_ArrDeinit(&panel->tabs);
	DS_FreeSlot(&g_panels, panel);
}

static void UpdateAndDrawUIPanelTree(UIPanel* panel, UI_Rect area_rect, bool splitter_is_hovered, UIPanel** out_hovered) {
	panel->this_frame_rect = area_rect;
	// panel->this_frame_root = NULL;

	if (panel->end_child[0] == NULL) {
		// leaf panel
		// let's make a tab panel.
		
		// hmm, should we use a box tree or no?
		// Box tree advantages:
		// - automatic fill/fit/etc layouting
		// - press/click animations
		// - keyboard navigation
		// Ultimately, in the best possible design, *either *case is painless.
		// So lets try with the box tree first!

		float tab_area_height = 28.f;
		
		UI_Rect content_rect = area_rect;
		content_rect.min.y += tab_area_height;

		UI_Key panel_key = UI_HashPtr(UI_KEY(), panel);
		UI_Vec2 panel_size = UI_RectSize(area_rect);
		
		// lets give a custom border color to the root

		UI_Box* root = UI_MakeRootBox(UI_KEY1(panel_key), panel_size.x, panel_size.y, UI_BoxFlag_DrawBorder);
		UIRegisterOrderedRoot(root);
		assert(root->parent == NULL);

		//UI_PushBox(root); // UI_PushRootBox(UI_KEY1(panel_key), area_rect.min, UI_SizePx(panel_size.x), UI_SizePx(panel_size.y), flags);

		if (!splitter_is_hovered && (UI_Pressed(root->key) || UI_PressedEx(root->key, UI_Input_MouseRight))) {
			g_active_panel = panel;
		}

		// maybe the active tab in active panel could be highlighted with some special color instead of this border color thing.
		// And maybe instead of Window -> menu for splitting, we could do right click menu (on empty region or tab) for splitting.
		// ALSO: tab buttons should have (very?) rounded corners only at the top, not bottom.
		if (g_active_panel == panel && panel->tabs.count == 0) {
			root->draw_args = UI_DrawBoxDefaultArgsInit();
			root->draw_args->border_color = ACTIVE_COLOR;
		}

		UI_BoxComputeRects(root, area_rect.min);
		UI_DrawBox(root);
		
		// tab area
		UI_Box* tab_area = UI_MakeRootBox(UI_KEY1(panel_key), panel_size.x, tab_area_height, UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder);
		UIRegisterOrderedRoot(tab_area);
		UI_PushBox(tab_area);
		
		//	UI_BoxComputeRects(panel->this_frame_root, panel->this_frame_rect.min);
		//	UI_DrawBox(panel->this_frame_root);

		int should_close_tab = -1;

		// Add tabs
		for (int i = 0; i < panel->tabs.count; i++) {
			UITab tab = panel->tabs.data[i];
			UI_Key tab_key = UI_HashInt(panel_key, i);
			
			STR_View tab_name;
			switch (tab.kind) {
				case UITabKind_Assets:      tab_name = STR_V("Assets"); break;
				case UITabKind_Properties:  tab_name = STR_V("Properties"); break;
				case UITabKind_Log:         tab_name = STR_V("Log"); break;
				case UITabKind_Custom:      tab_name = tab.custom_tab_name; break;
			}
			
			UI_BoxFlags button_flags = UI_BoxFlag_Horizontal | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder;
			if (i == panel->active_tab) {
				button_flags |= UI_BoxFlag_DrawTransparentBackground;
			}

			UI_Box* tab_button = UI_AddBox(UI_KEY1(tab_key), UI_SizeFit(), UI_SizeFit(), button_flags);
			
			if (i == panel->active_tab && panel == g_active_panel) {
				tab_button->draw_args = UI_DrawBoxDefaultArgsInit();
				tab_button->draw_args->transparent_bg_color = ACTIVE_COLOR;
			}

			UI_PushBox(tab_button);
			UI_AddBoxWithText(UI_KEY1(tab_key), UI_SizeFit(), UI_SizeFit(), 0, tab_name);

			UI_BoxFlags close_flags = UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_HasText;
			UI_Box* tab_close_button = UI_AddBoxWithTextC(UI_KEY1(tab_key), UI_SizeFit(), UI_SizeFit(), close_flags, "J");
			tab_close_button->font = UI_STATE.icons_font;

			if (UI_Clicked(tab_close_button->key)) {
				should_close_tab = i;
			}

			UI_PopBox(tab_button);

			if (UI_PressedIdle(tab_button->key)) { // bring to view
				g_active_panel = panel;
				panel->active_tab = i;
			}
		}

		UI_PopBox(tab_area);
		UI_BoxComputeRects(tab_area, area_rect.min);
		UI_DrawBox(tab_area);

		if (should_close_tab != -1) {
			FreeUITab(&panel->tabs.data[should_close_tab]);
			DS_ArrRemove(&panel->tabs, should_close_tab);
			if (panel->active_tab > should_close_tab) panel->active_tab -= 1;
			panel->active_tab = UI_Min(panel->active_tab, panel->tabs.count - 1); // clamp the active tab index
		}

		if (panel->tabs.count > 0) {
			UITab tab = DS_ArrGet(panel->tabs, panel->active_tab);
			switch (tab.kind) {
				case UITabKind_Assets:      UIAssetsBrowserTab(panel_key, content_rect); break;
				case UITabKind_Properties:  UIPropertiesTab(panel_key, content_rect); break;
				case UITabKind_Log:         UILogTab(panel_key, content_rect); break;
				case UITabKind_Custom: {
					bool found = false;
					for (int i = 0; i < g_custom_tab_kinds.count; i++) {
						TempRegisteredCustomTab tab_kind = g_custom_tab_kinds.data[i];
						if (tab_kind.tab_id == tab._custom_tab_id) {
							found = true;
							HT_TabPlacement placement = {0};
							placement.tab_id = tab_kind.tab_id;
							placement.area_rect = HT_Rect{{content_rect.min.x, content_rect.min.y}, {content_rect.max.x, content_rect.max.y}};
							DS_ArrPush(&g_custom_tab_placements, placement);
						}
					}
					if (!found) {
						UI_AddBoxWithTextC(UI_KEY1(tab._custom_tab_id), UI_SizeFit(), UI_SizeFit(), 0, "This tab does not have an owning plugin anymore.");
					}
				} break;
			}
		}
		
		// UI_PopBox(root);
		
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
		for (UIPanel* child = panel->end_child[0]; child; child = child->link[1]) {
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
		UIPanel* prev = NULL;
		for (UIPanel* child = panel->end_child[0]; child; child = child->link[1]) {
			assert(prev == child->link[0]); // tree validation
			assert(child->parent == panel); // tree validation

			// hmm... how can you click a rectangle but also splitter?
			float end = splitters_state->panel_end_offsets[i];

			//child->size = end - start; // write back the new panel size
		
			UI_Rect rect = area_rect;
			rect.min._[X] = area_rect.min._[X] + start + 1.f;
			rect.max._[X] = area_rect.min._[X] + end - 1.f;
			UpdateAndDrawUIPanelTree(child, rect, splitter_is_hovered, out_hovered);
		
			start = end;
			i++;
			prev = child;
		}
	}
}

// why does this need to be in a post pass?
static void PostUpdateUIPanelTree(UIPanel* panel) {
	if (panel->end_child[0] == NULL) {
		// leaf panel
		
		/*
		UI_Key panel_key = UI_HashPtr(UI_KEY(), panel);

		if (panel->tabs.count > 0) {
			UITab tab = DS_ArrGet(panel->tabs, panel->active_tab);
			switch (tab.kind) {
			case UITabKind_Assets:      AssetsBrowserTab(panel_key); break;
			case UITabKind_Properties:  PropertiesTab(panel_key); break;
			case UITabKind_Log:         LogTab(panel_key); break;
			case UITabKind_Custom: {
				bool found = false;
				for (int i = 0; i < g_custom_tab_kinds.count; i++) {
					TempRegisteredCustomTab tab_kind = g_custom_tab_kinds.data[i];
					if (tab_kind.id == tab.custom_tab_id) {
						// save the rectangle for this tab ID for the next frame?
						// hmm, which order should these things happen?
						// We could iterate the panel tree and update input in PreUpdate
						found = true;
					}
				}
				if (!found) {
					UI_AddBoxWithText(UI_KEY1(tab.custom_tab_id), UI_SizeFit(), UI_SizeFit(), STR_("This tab does not have an owning plugin anymore."));
				}
			} break;
			}
		}*/

		//if (panel->this_frame_root) {
		//	UI_BoxComputeRects(panel->this_frame_root, panel->this_frame_rect.min);
		//	UI_DrawBox(panel->this_frame_root);
		//}
	}
	else {
		//for (UIPanel* child = panel->end_child[0]; child; child = child->link[1]) {
		//	DrawUIPanelTree(child);
		//}
	}
}

// Split panel and return the split
static UIPanel* SplitPanel(UIPanel* panel, UI_Axis split_along) {
	assert(panel->end_child[0] == NULL); // panel must be a leaf
	UIPanel* parent = panel->parent;
	UIPanel* split = NULL;

	if (parent && parent->split_along == split_along) {
		// Add one more panel at the end
		split = parent;
		UIPanel* new_panel = NewUIPanel();

		new_panel->parent = split;
		if (split->end_child[0]) split->end_child[1]->link[1] = new_panel;
		else split->end_child[0] = new_panel;
		new_panel->link[0] = split->end_child[1];
		split->end_child[1] = new_panel;

		new_panel->size = panel->size;
		// g_active_panel = new_panel;
	}
	else {
		// Split the active panel and insert itself as the first child of the split
		split = NewUIPanel();
		split->size = panel->size;
		split->split_along = split_along;

		UIPanel* new_leaf = NewUIPanel();
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
			g_root_panel = split;
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

static void ClosePanel(UIPanel* panel) {
	assert(panel->end_child[0] == NULL); // panel must be a leaf
	
	UIPanel* parent = panel->parent;
	DS_ArrClear(&panel->tabs);
	if (parent == NULL) return;

	// Remove panel from its parents child list
	if (panel->link[0]) panel->link[0]->link[1] = panel->link[1];
	else parent->end_child[0] = panel->link[1];
	if (panel->link[1]) panel->link[1]->link[0] = panel->link[0];
	else parent->end_child[1] = panel->link[0];
	
	FreeUIPanel(panel);
	
	assert(parent->end_child[0] != NULL); // child lists must always have more than 1 element

	// if the parent now has only one child, then replace the parent with the last child
	if (parent->end_child[0] && parent->end_child[0] == parent->end_child[1]) {
		UIPanel* last_child = parent->end_child[0];
		
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
			g_root_panel = last_child;
		}
		FreeUIPanel(parent);
	}
}

static void AddNewTabToActivePanel(UITab tab) {
	if (g_active_panel) {
		g_active_panel->active_tab = g_active_panel->tabs.count;
		DS_ArrPush(&g_active_panel->tabs, tab);
	}
}

static void UILogTab(UI_Key key, UI_Rect content_rect) {
}
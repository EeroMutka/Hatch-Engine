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
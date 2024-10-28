
// so what kind of data do we want?
// - int
// - float
// - text

// The asset tree is essentially a string tree.
// A scene view is also a string tree.
// A DataTree is a tree of nodes. Each node has N inline values of potentially varying types. But N must be the same for all elements of the tree, to make splitters possible.
// There should be no data-table like keyboard navigation with arrow keys - only with tab & shift-tab when in edit mode.
// hmm, but when we have a struct edit, then "enter" should edit the value, not the name. So in DataTrees, there is one canonical value.

// In this case, we can just use the UI-defined data structure as the basis for storing the data.
// I think that's fine. Either that, or we have a "basic tree" header which contains just regular tree relations and user data.
// That way we can have a hatch plugin that doesn't touch "UI code" still iterate through the data tree. I think in this case we just want duplicate data. OR: we have like a function pointer for querying things!

struct UI_DataTreeNode {
	UI_DataTreeNode* parent;
	UI_DataTreeNode* prev;
	UI_DataTreeNode* next;
	UI_DataTreeNode* first_child;
	UI_DataTreeNode* last_child;
	
	UI_Key key;
	bool* is_open_ptr;
};

struct UI_DataTreeState {
	UI_Key selection;
	UI_Key pressing;
	bool moved_this_frame;
	
	// To check for drag-n-drop, you can check if the mouse has been released this frame and use the `drag_n_dropping` key.
	// It doesn't matter if you do this check before or after calling UI_AddDataTree.
	UI_Key drag_n_dropping;
	UI_Key drag_n_dropping_new;
	
	UI_Vec2 mouse_travel_after_press;
};

struct UI_DataTree {
	int num_columns;
	UI_DataTreeNode* root;
	bool allow_selection;
	bool allow_drag_n_drop;

	void (*AddValueUI)(UI_DataTree* tree, UI_Box* parent, UI_DataTreeNode* node, int row, int column);
	
	void* user_data; // unused by the library

	// TODO: function pointer for drawing an icon
	// TODO: function pointer for adding UI for inline value [N]
	// TODO: function pointer for add-full-custom-UI. This would mean not displaying the name or splitters. This could be used for embedding custom UI, e.g. an image viewer

	// UI-state -- keep it here for now, but really we might want multiple UI tree instances showing the same tree
};

UI_API void UI_AddDataTree(UI_Box* box, UI_Size w, UI_Size h, UI_DataTree* tree, UI_DataTreeState* state);


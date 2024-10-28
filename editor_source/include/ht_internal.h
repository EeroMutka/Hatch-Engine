
#include "../plugin_include/hatch_api.h"

// Convention for marking non-static / exported symbols
#define EXPORT

#define TODO() __debugbreak()

#define UI_CUSTOM_VEC2
typedef vec2 UI_Vec2;
#include "../fire/fire_ui/fire_ui.h"

#include "../fire/fire_os_window.h"
#include "../fire/fire_os_clipboard.h"

#define BUILD_API
#define FIRE_BUILD_NO_IMPLEMENTATION
#include "../fire/fire_build.h"

#define STR_USE_FIRE_DS
#include "../fire/fire_string.h"

#include "../utils/ui_data_tree.h"
#include "../utils/os_misc.h"
#include "../utils/os_directory_watch.h"

// -- Globals ---------------------------------------------------------

extern DS_Arena* TEMP;
#define MEM_SCOPE(ARENA) &DS_MemScope{ ARENA, TEMP }
#define MEM_TEMP()       &DS_MemTemp{ TEMP }

// -- Constants -------------------------------------------------------

static const float TOP_BAR_HEIGHT = 28.f;
static const UI_Color ACTIVE_COLOR = {180, 120, 50, 230};
static const vec2 DEFAULT_UI_INNER_PADDING = {12.f, 12.f};
static const float VAR_SPLITTER_AREA_HALF_WIDTH = 5.f;

// -- ht_data_model.cpp -----------------------------------------------

struct PluginIncludeStamp {
	u64 last_write_time;
	STR_View file_path;
};

struct ActivePluginData {
	DS_Arena arena;
	//Asset* plugin_asset;
	DS_Map(uint64_t, PluginIncludeStamp) includes;
};

enum TypeKind {
	TypeKind_Float,
	TypeKind_Int,
	TypeKind_Bool,
	TypeKind_AssetRef,
	TypeKind_Struct,
	TypeKind_Array,
	TypeKind_String,
	TypeKind_Type,
	TypeKind_COUNT,
	TypeKind_INVALID,
};

struct Array {
	void* data;
	i32 count;
	i32 capacity;
};

struct Asset;
struct AssetRef {
	Asset* asset;
	uint64_t generation;
};

struct Type {
	TypeKind kind;
	TypeKind subkind; // for arrays, this is the element type
	AssetRef _struct;
};

//EXPORT STR_View TypeToString(Type* type);
EXPORT STR_View TypeKindToString(TypeKind type);

enum AssetKind {
	AssetKind_Package, // Packages are root-level folders. Packages can be saved to disk. Packages cannot contain other packages.
	AssetKind_Folder,
	AssetKind_C,
	AssetKind_CPP,
	AssetKind_Plugin,
	AssetKind_StructType,
	AssetKind_StructData,
};

struct String {
	UI_Text text; // for now, use UI_Text. This is just a quick thing to get started
};

struct StructMember {
	String name;
	Type type;
	i32 offset;
};

struct Asset_StructType {
	DS_DynArray(StructMember) members;
	i32 size;
	i32 alignment;
	//DS_Set(Asset*) uses_struct_types; // recursively contains all struct types this contains
};

struct PluginOptions {
	Array SourceFiles; // Array<AssetRef>
	AssetRef Data;
};

struct Asset_Plugin {
	PluginOptions options; // a value of type g_plugin_options_struct_type
	OS_DLL* dll_handle;
	void (*dll_UpdatePlugin)(HT_API* HT);
};

struct Asset_StructData {
	AssetRef struct_type;

	// When editing a type, just loop through all structs of that type and update the data layout.
	// ... When changing a member type in the struct type, removing a member, or destroying the struct type asset, that is a destructive change.
	//     But we don't want to destroy the data. TODO: we should have an additional array of "stale_members" which stores the name, type and value of old members.
	//     The stale array could then be serialized as regular.
	void* data;
};

#define AssetSlotIsEmpty(ASSET)  ((ASSET)->generation == 0)
#define AssetSlotSetEmpty(ASSET) ((ASSET)->generation = 0)

struct Asset {
	AssetKind kind;
	UI_Text name; // not used for AssetKind_Package

	Asset* parent;
	Asset* prev;
	Asset* next;
	Asset* first_child;
	Asset* last_child;

	uint64_t generation;

	// For packages, filesys_path stores the full absolute path.
	uint64_t filesys_modtime;
	STR_View package_filesys_path;

	union {
		Asset_StructType struct_type;
		Asset_StructData struct_data;
		Asset_Plugin plugin;
	};

	STR_View reload_assets_filesys_path; // temporary variable
	bool reload_assets_pass2_needs_hotreload; // temporary variable

	bool ui_state_is_open; // for the Assets panel
};

struct AssetTree {
	DS_SlotAllocator(Asset, 16) assets;
	uint64_t next_asset_generation;
	Asset* root;

	// built-in types
	Asset* plugin_options_struct_type;  // built-in custom struct type for PluginOptions
	Asset* name_and_type_struct_type;   // built-in custom struct type for NameAndType
};

// Pass STR_View into printf-style %.*s arguments
#define StrArg(X) X.size, X.data

EXPORT bool AssetIsValid(AssetRef handle);

EXPORT AssetRef GetAssetHandle(Asset* asset);

EXPORT Asset* MakeNewAsset(AssetTree* tree, AssetKind kind);

EXPORT void InitStructDataAsset(Asset* asset, Asset* struct_type);

//EXPORT void StructMemberNodeInit(StructMemberNode* node);
//EXPORT void StructMemberNodeDeinit(StructMemberNode* node);
//EXPORT int StructMemberNodeFindMemberIndex(StructMemberNode* node);

EXPORT void GetTypeSizeAndAlignment(Type* type, i32* out_size, i32* out_alignment);
EXPORT void ComputeStructLayout(Asset* struct_type);

EXPORT void StructTypeAddMember(AssetTree* tree, Asset* struct_type);

// if `move_before_this` is NULL, the asset is added at the end of the root
EXPORT void MoveAssetToBefore(AssetTree* tree, Asset* asset, Asset* move_before_this);

// if `move_before_this` is NULL, the asset is added at the beginning of the root
EXPORT void MoveAssetToAfter(AssetTree* tree, Asset* asset, Asset* move_after_this);

EXPORT void MoveAssetToInside(AssetTree* tree, Asset* asset, Asset* move_inside_this);

EXPORT void DeleteAssetIncludingChildren(AssetTree* tree, Asset* asset);

EXPORT void ArrayPush(Array* array, i32 elem_size);
EXPORT void ArrayClear(Array* array, i32 elem_size);

EXPORT void StructMemberInit(StructMember* x);
EXPORT void StructMemberDeinit(StructMember* x);
EXPORT void StringInit(String* x);
EXPORT void StringDeinit(String* x);

// - returns NULL if not found
EXPORT Asset* FindAssetFromPath(Asset* package, STR_View path);

// -- ht_log.cpp ------------------------------------------------------

struct LogMessage {
	int index;
	STR_View string;
};

EXPORT void LogPrint(STR_View str);

// -- ht_plugin_compiler.cpp ------------------------------------------

EXPORT void RecompilePlugin(Asset* plugin, STR_View hatch_install_directory);

// -- ht_serialize.cpp ------------------------------------------------

EXPORT STR_View GetAssetFileExtension(Asset* asset);
EXPORT void SavePackageToDisk(Asset* package);
EXPORT Asset* LoadPackageFromDisk(AssetTree* tree, STR_View path);

// -- ui.cpp ----------------------------------------------------------

struct UI_Tab; // Placeholder for the user

struct UI_Panel {
	UI_Panel* parent;
	UI_Panel* end_child[2]; // 0 is first, 1 is last
	UI_Panel* link[2];      // 0 is prev, 1 is next

	float size; // not currently used
	UI_Axis split_along;

	int active_tab;
	DS_DynArray(UI_Tab*) tabs;

	//UI_Rect this_frame_rect;

	//UI_Box *this_frame_root;
};

struct UI_PanelTree {
	DS_SlotAllocator(UI_Panel, 16) panels;
	UI_Panel* root;
	UI_Panel* active_panel; // NULL by default

	STR_View (*query_tab_name)(UI_PanelTree* tree, UI_Tab* tab);
	void (*update_and_draw_tab)(UI_PanelTree* tree, UI_Tab* tab, UI_Key key, UI_Rect area_rect);

	void* user_data;
};

struct UIDropdownState {
	UI_Key deepest_hovered_root = UI_INVALID_KEY;
	UI_Key deepest_hovered_root_new = UI_INVALID_KEY;
	bool has_added_deepest_hovered_root = false;
};

EXPORT void UI_PanelTreeInit(UI_PanelTree* tree, DS_Allocator* allocator);
EXPORT void UI_PanelTreeUpdateAndDraw(UI_PanelTree* tree, UI_Panel* panel, UI_Rect area_rect, bool splitter_is_hovered, UI_Panel** out_hovered);

EXPORT UI_Panel* NewUIPanel(UI_PanelTree* tree);
EXPORT void FreeUIPanel(UI_PanelTree* tree, UI_Panel* panel);

EXPORT void UIDropdownStateBeginFrame(UIDropdownState* s);

EXPORT bool UIOrderedDropdownShouldClose(UIDropdownState* s, UI_Box* box);

EXPORT void UIRegisterOrderedRoot(UIDropdownState* s, UI_Box* dropdown);

EXPORT void UIAddAssetIcon(UI_Box* box, Asset* asset);

EXPORT void UIAddPadY(UI_Box* box);
EXPORT void UIPushDropdown(UIDropdownState* s, UI_Box* box, UI_Size w, UI_Size h);
EXPORT void UIPopDropdown(UI_Box* box);

EXPORT void UIAddTopBarButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string);
EXPORT void UIAddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string);

EXPORT UI_Panel* SplitPanel(UI_PanelTree* tree, UI_Panel* panel, UI_Axis split_along);
EXPORT void ClosePanel(UI_PanelTree* tree, UI_Panel* panel);
EXPORT void AddNewTabToActivePanel(UI_PanelTree* tree, UI_Tab tab);

// -- ht_editor.cpp ---------------------------------------------------

struct RenderState;

enum TabKind {
	TabKind_Assets,
	TabKind_Properties,
	TabKind_Log,
};

struct UI_Tab {
	TabKind kind;
};

struct PerFrameState {
	UI_Panel* hovered_panel;
	bool file_dropdown_open;
	bool panel_dropdown_open;
	bool edit_dropdown_open;

	UI_Box* type_dropdown_button;
	UI_Box* type_dropdown;
};

struct EditorState {
	DS_Arena persistent_arena;
	DS_Arena temporary_arena;

	ivec2 window_size = {1280, 720};
	OS_WINDOW window;

	UI_Inputs ui_inputs;
	UI_FontIndex base_font, icons_font;

	UI_Text dummy_text;
	UI_Text dummy_text_2;

	DS_Arena log_arenas[2];
	DS_DynArray(LogMessage) log_messages[2];
	int next_log_message_index;
	int current_log_arena = 0;

	AssetTree asset_tree;
	UI_DataTreeState assets_tree_ui_state;

	UI_DataTreeState properties_tree_type_ui_state;
	UI_DataTreeState properties_tree_data_ui_state;

	UIDropdownState dropdown_state;

	DS_SlotAllocator(UI_Tab, 8) all_tabs;
	UI_PanelTree panel_tree;

	// ------------------
	
	vec2 rmb_menu_pos;
	TabKind rmb_menu_tab_kind;
	bool rmb_menu_open;

	UI_Key type_dropdown_open = UI_INVALID_KEY;
	
	// ------------------

	STR_View hatch_install_directory;

	RenderState* render_state;

	PerFrameState frame; // cleared at the beginning of a frame
};

EXPORT void AddTopBar(EditorState* s);
EXPORT void UIAssetsBrowserTab(EditorState* s, UI_Key key, UI_Rect content_rect);
EXPORT void UIPropertiesTab(EditorState* s, UI_Key key, UI_Rect content_rect);
EXPORT void UILogTab(EditorState* s, UI_Key key, UI_Rect content_rect);

EXPORT void UpdateAndDrawDropdowns(EditorState* s);
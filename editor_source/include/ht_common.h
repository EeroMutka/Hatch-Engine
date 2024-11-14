
#include "../plugin_include/hatch_api.h"

// Convention for marking non-static / exported symbols
#define EXPORT

#ifdef DISABLE_ASSERT
#define ASSERT(X)
#else
#define ASSERT(X) do { if (!(X)) __debugbreak(); } while (0)
#endif

#define TODO() __debugbreak()

#define DS_ASSERT(X) ASSERT(X)
#include <fire_ds.h>
#include <fire_string.h>

#define UI_CUSTOM_VEC2
typedef vec2 UI_Vec2;
#include <fire_ui/fire_ui.h>

#define OS_WINDOW_API extern "C"
#include <fire_os_window.h>

#define OS_CLIPBOARD_API extern "C"
#include <fire_os_clipboard.h>

#define BUILD_API extern "C"
#include <fire_build.h>

#include "../utils/ui_data_tree.h"

#define OS_API extern "C"
#include "../utils/os_misc.h"
#include "../utils/os_directory_watch.h"

// -- Utility macros --------------------------------------------------

// Allocate a slot from a bucket array with a freelist
#define NEW_SLOT(OUT_SLOT, BUCKET_ARRAY, FIRST_FREE_SLOT, NEXT) \
	if (*FIRST_FREE_SLOT) { \
		*OUT_SLOT = *FIRST_FREE_SLOT; \
		*FIRST_FREE_SLOT = (*FIRST_FREE_SLOT)->NEXT; \
	} else { \
		*(void**)OUT_SLOT = DS_BucketArrayPushUndef(BUCKET_ARRAY); \
	}

// Free a slot from a bucket array with a freelist
#define FREE_SLOT(SLOT, FIRST_FREE_SLOT, NEXT) \
	SLOT->NEXT = *FIRST_FREE_SLOT; \
	*FIRST_FREE_SLOT = SLOT;

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

struct Asset;

#define ASSET_FREELIST_END 0xFFFFFFFFFFFFFFFF

union AssetHandleDecoded {
	struct {
		u16 bucket_index;
		u16 elem_index;
		u32 generation;
	};
	u64 val;
};

static inline HT_AssetHandle EncodeAssetHandle(AssetHandleDecoded handle) { return *(HT_AssetHandle*)&handle; }
static inline AssetHandleDecoded DecodeAssetHandle(HT_AssetHandle handle) { return *(AssetHandleDecoded*)&handle; }

#define AssetKind_FreeSlot (AssetKind)0
enum AssetKind {
	AssetKind_Root = 1,
	AssetKind_Package, // Packages are root-level folders. Packages can be saved to disk. Packages cannot contain other packages.
	AssetKind_Folder,
	//AssetKind_C,
	//AssetKind_CPP,
	AssetKind_File, // file with an arbitrary extension. The extension is encoded in the asset name.
	AssetKind_Plugin,
	AssetKind_StructType,
	AssetKind_StructData,
};

struct String {
	UI_Text text; // for now, use UI_Text. This is just a quick thing to get started
};

struct StructMember {
	String name;
	HT_Type type;
	i32 offset;
};

struct Asset_StructType {
	DS_DynArray(StructMember) members;
	i32 size;
	i32 alignment;
	HT_AssetHandle asset_viewer_registered_by_plugin;
	//DS_Set(Asset*) uses_struct_types; // recursively contains all struct types this contains
};

struct Asset_Package {
	STR_View filesys_path;
	OS_DirectoryWatch dir_watch;
};

struct PluginOptions {
	HT_Array source_files; // Array<AssetRef>
	HT_AssetHandle data;
};

struct PluginAllocationHeader {
	u32 allocation_index;
	size_t size;
};

struct Asset_Plugin {
	PluginOptions options; // a value of type g_plugin_options_struct_type
	
	OS_DLL* dll_handle;
	
	void (*UpdatePlugin)(HT_API* HT);
	void (*LoadPlugin)(HT_API* HT);
	void (*UnloadPlugin)(HT_API* HT);

	DS_DynArray(PluginAllocationHeader*) allocations;
};

struct Asset_StructData {
	HT_AssetHandle struct_type;

	// When editing a type, just loop through all structs of that type and update the data layout.
	// ... When changing a member type in the struct type, removing a member, or destroying the struct type asset, that is a destructive change.
	//     But we don't want to destroy the data. TODO: we should have an additional array of "stale_members" which stores the name, type and value of old members.
	//     The stale array could then be serialized as regular.
	void* data;
};

struct Asset {
	AssetKind kind;
	UI_Text name; // not used for AssetKind_Package
	HT_AssetHandle handle;
	u64 modtime;

	Asset* parent;
	Asset* prev;
	Asset* next;
	Asset* first_child;
	Asset* last_child;

	union {
		AssetHandleDecoded freelist_next; // end of the list is marked with ASSET_FREELIST_END
		Asset_StructType struct_type;
		Asset_StructData struct_data;
		Asset_Plugin plugin;
		Asset_Package package;
	};

	STR_View reload_assets_filesys_path; // temporary variable
	bool reload_assets_pass2_needs_hotreload; // temporary variable

	bool ui_state_is_open; // for the Assets panel
};

#define ASSETS_PER_BUCKET 32

struct AssetBucket {
	Asset assets[ASSETS_PER_BUCKET];
};

struct AssetTree {
	DS_DynArray(AssetBucket*) asset_buckets;
	int assets_num_allocated;
	AssetHandleDecoded first_free_asset; // ASSET_FREELIST_END

	Asset* root;

	// built-in types
	Asset* plugin_options_struct_type;  // built-in custom struct type for PluginOptions
	Asset* name_and_type_struct_type;   // built-in custom struct type for NameAndType
};

// Pass STR_View into printf-style %.*s arguments
#define StrArg(X) (int)X.size, X.data

EXPORT STR_View HT_TypeKindToString(HT_TypeKind type);
EXPORT HT_TypeKind StringToTypeKind(STR_View str); // may return HT_TypeKind_INVALID

EXPORT bool AssetIsValid(AssetTree* tree, HT_AssetHandle handle);
EXPORT Asset* GetAsset(AssetTree* tree, HT_AssetHandle handle); // returns NULL if the handle is invalid

EXPORT Asset* MakeNewAsset(AssetTree* tree, AssetKind kind);

EXPORT void InitStructDataAsset(AssetTree* tree, Asset* asset, Asset* struct_type);
EXPORT void DeinitStructDataAssetIfInitialized(AssetTree* tree, Asset* asset);

//EXPORT void StructMemberNodeInit(StructMemberNode* node);
//EXPORT void StructMemberNodeDeinit(StructMemberNode* node);
//EXPORT int StructMemberNodeFindMemberIndex(StructMemberNode* node);

EXPORT void GetTypeSizeAndAlignment(AssetTree* tree, HT_Type* type, i32* out_size, i32* out_alignment);
EXPORT void ComputeStructLayout(AssetTree* tree, Asset* struct_type);

EXPORT void StructTypeAddMember(AssetTree* tree, Asset* struct_type);

// if `move_before_this` is NULL, the asset is added at the end of the root
EXPORT void MoveAssetToBefore(AssetTree* tree, Asset* asset, Asset* move_before_this);

// if `move_before_this` is NULL, the asset is added at the beginning of the root
EXPORT void MoveAssetToAfter(AssetTree* tree, Asset* asset, Asset* move_after_this);

EXPORT void MoveAssetToInside(AssetTree* tree, Asset* asset, Asset* move_inside_this);

EXPORT void DeleteAssetIncludingChildren(AssetTree* tree, Asset* asset);

EXPORT void Construct(AssetTree* tree, void* data, HT_Type* type);
EXPORT void Destruct(AssetTree* tree, void* data, HT_Type* type);

EXPORT void ArrayPush(HT_Array* array, i32 elem_size);
EXPORT void ArrayClear(HT_Array* array, i32 elem_size);
EXPORT void ArrayDeinit(HT_Array* array);

EXPORT void ItemGroupInit(HT_ItemGroup* group);
EXPORT void ItemGroupDeinit(HT_ItemGroup* group, i32 item_full_size);
EXPORT HT_ItemIndex ItemGroupAdd(HT_ItemGroup* group, i32 item_full_size); // does not insert the asset into the list yet, you must call MoveItemToAfter
EXPORT void ItemGroupRemove(HT_ItemGroup* group, HT_ItemIndex item, i32 item_full_size);

EXPORT void CalculateItemOffsets(i32 item_size, i32 item_align, i32* out_item_offset, i32* out_item_full_size);

// `item` may be HT_ITEM_INDEX_INVALID, in which case NULL is returned. Otherwise the index must be valid.
EXPORT HT_ItemHeader* GetItemFromIndex(HT_ItemGroup* group, HT_ItemIndex item, i32 item_full_size);

// if `move_after_this` is HT_ITEM_INDEX_INVALID, the item is moved to the beginning of the list.
EXPORT void MoveItemToAfter(HT_ItemGroup* group, HT_ItemIndex item, HT_ItemIndex move_after_this, i32 item_full_size);

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

// -- ht_serialize.cpp ------------------------------------------------

EXPORT STR_View AssetGetFilepath(DS_Arena* arena, Asset* asset);

EXPORT void HotreloadPackages(AssetTree* tree);

EXPORT STR_View GetAssetFileExtension(Asset* asset);

EXPORT void SavePackageToDisk(Asset* package);

EXPORT Asset* LoadPackageFromDisk(AssetTree* tree, STR_View path);

// -- ui.cpp ----------------------------------------------------------

struct UI_Tab; // Placeholder for the user

struct UI_Panel {
	union {
		UI_Panel* parent;
		UI_Panel* freelist_next;
	};
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
	DS_BucketArray(UI_Panel) panels;
	UI_Panel* first_free_panel;
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

EXPORT void UI_AddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string, UI_Font icons_font);

EXPORT void UI_PanelTreeInit(UI_PanelTree* tree, DS_Allocator* allocator);
EXPORT void UI_PanelTreeUpdateAndDraw(UIDropdownState* s, UI_PanelTree* tree, UI_Panel* panel, UI_Rect area_rect, bool splitter_is_hovered, UI_Font icons_font, UI_Panel** out_hovered);

EXPORT UI_Panel* NewUIPanel(UI_PanelTree* tree);
EXPORT void FreeUIPanel(UI_PanelTree* tree, UI_Panel* panel);

EXPORT void UIDropdownStateBeginFrame(UIDropdownState* s);

EXPORT bool UIOrderedDropdownShouldClose(UIDropdownState* s, UI_Box* box);

EXPORT void UIRegisterOrderedRoot(UIDropdownState* s, UI_Box* dropdown);

EXPORT void UIAddAssetIcon(UI_Box* box, Asset* asset, UI_Font icons_font);

EXPORT void UIAddPadY(UI_Box* box);
EXPORT void UIPushDropdown(UIDropdownState* s, UI_Box* box, UI_Size w, UI_Size h);
EXPORT void UIPopDropdown(UI_Box* box);

EXPORT void UIAddTopBarButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string);
EXPORT void UIAddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string);

EXPORT UI_Panel* SplitPanel(UI_PanelTree* tree, UI_Panel* panel, UI_Axis split_along);
EXPORT void ClosePanel(UI_PanelTree* tree, UI_Panel* panel);
EXPORT void AddNewTabToActivePanel(UI_PanelTree* tree, UI_Tab* tab);

// -- ht_editor.cpp ---------------------------------------------------

struct RenderState;

struct UI_Tab {
	STR_View name; // empty string means free slot
	HT_AssetHandle owner_plugin;
	UI_Tab* freelist_next;
};

struct PerFrameState {
	UI_Panel* hovered_panel;
	bool file_dropdown_open;
	bool edit_dropdown_open;

	UI_Box* window_dropdown;
	UI_Box* window_dropdown_button;
	
	UI_Box* type_dropdown;
	UI_Box* type_dropdown_button;

	DS_DynArray(HT_AssetViewerTabUpdate) queued_asset_viewer_tab_updates;
	DS_DynArray(HT_CustomTabUpdate) queued_custom_tab_updates;
};

struct EditorState {
	HT_API* api;

	DS_Arena persistent_arena;
	DS_Arena temporary_arena;

	ivec2 window_size = {1280, 720};
	OS_Window window;

	HT_InputFrame input_frame;
	
	UI_Inputs ui_inputs;
	UI_Font default_font;
	UI_Font icons_font;

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

	DS_BucketArray(UI_Tab) tab_classes;
	UI_Tab* first_free_tab_class;
	
	UI_Tab* properties_tab_class;
	UI_Tab* assets_tab_class;
	UI_Tab* asset_viewer_tab_class;
	UI_Tab* log_tab_class;

	UI_PanelTree panel_tree;

	// ------------------
	
	vec2 rmb_menu_pos;
	UI_Tab* rmb_menu_tab_class;
	bool rmb_menu_open;

	bool window_dropdown_open;
	UI_Key type_dropdown_open = UI_INVALID_KEY;
	
	// ------------------

	STR_View hatch_install_directory;

	RenderState* render_state;

	PerFrameState frame; // cleared at the beginning of a frame
};

EXPORT UI_Tab* CreateTabClass(EditorState* s, STR_View name);
EXPORT void DestroyTabClass(EditorState* s, UI_Tab* tab);

EXPORT void AddTopBar(EditorState* s);
EXPORT void UIAssetsBrowserTab(EditorState* s, UI_Key key, UI_Rect content_rect);
EXPORT void UIPropertiesTab(EditorState* s, UI_Key key, UI_Rect content_rect);
EXPORT void UILogTab(EditorState* s, UI_Key key, UI_Rect content_rect);

EXPORT void InitAPI(EditorState* s);

EXPORT void LoadPlugin(EditorState* s, Asset* plugin);
EXPORT void UnloadPlugin(EditorState* s, Asset* plugin);

EXPORT void UpdatePlugins(EditorState* s);
EXPORT void BuildPluginD3DCommandLists(EditorState* s);

EXPORT void UpdateAndDrawDropdowns(EditorState* s);

// -- ht_plugin_compiler.cpp ------------------------------------------

EXPORT void RecompilePlugin(EditorState* s, Asset* plugin, STR_View hatch_install_directory);

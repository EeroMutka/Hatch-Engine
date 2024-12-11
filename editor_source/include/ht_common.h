
#define HT_NO_STATIC_PLUGIN_EXPORTS

#ifdef HT_DYNAMIC
#define HT_IMPORT extern "C" __declspec(dllexport)
#endif // otherwise HT_IMPORT is defined to be empty in the project file defines list

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
#include <ht_utils/fire/fire_ds.h>

#define STR_ASSERT(X) ASSERT(X)
#define STR_CUSTOM_VIEW_TYPE
typedef HT_StringView STR_View;
#include <ht_utils/fire/fire_string.h>

#define UI_CUSTOM_VEC2
typedef vec2 UI_Vec2;
#include <ht_utils/fire/fire_ui/fire_ui.h>

#define OS_WINDOW_API extern "C"
#include <ht_utils/fire/fire_os_window.h>

#define OS_TIMING_API extern "C"
#include <ht_utils/fire/fire_os_timing.h>

#define OS_CLIPBOARD_API extern "C"
#include <ht_utils/fire/fire_os_clipboard.h>

#define BUILD_API extern "C"
#include <ht_utils/fire/fire_build.h>

#include "../utils/ui_data_tree.h"

#define OS_API extern "C"
#include "../utils/os_misc.h"
#include "../utils/os_directory_watch.h"

#include <stdio.h> // for printf, this should be removed in the final product

// -- Utility macros --------------------------------------------------

// Allocate a slot from a bucket array with an index-based freelist.
// Zero-initializes new elements, but keeps old data on reuse.
#define NEW_SLOT(OUT_INDEX, BUCKET_ARRAY, FIRST_FREE_SLOT, NEXT) \
	if (*FIRST_FREE_SLOT) { \
		*OUT_INDEX = *FIRST_FREE_SLOT; \
		*FIRST_FREE_SLOT = DS_BkArrGet(BUCKET_ARRAY, *FIRST_FREE_SLOT)->NEXT; \
	} else { \
		DS_BkArrPushZero(BUCKET_ARRAY); \
		*OUT_INDEX = DS_BkArrLast(BUCKET_ARRAY); \
	}

// Free a slot from a bucket array with an index-based freelist
#define FREE_SLOT(INDEX, BUCKET_ARRAY, FIRST_FREE_INDEX, NEXT) \
	DS_BkArrGet(BUCKET_ARRAY, INDEX)->NEXT = *FIRST_FREE_INDEX; \
	*FIRST_FREE_INDEX = INDEX;

// Allocate a slot from a bucket array with a pointer-based freelist
// Zero-initializes new elements, but keeps old data on reuse.
#define NEW_SLOT_PTR(OUT_SLOT, BUCKET_ARRAY, FIRST_FREE_SLOT, NEXT) \
	if (*FIRST_FREE_SLOT) { \
		*OUT_SLOT = *FIRST_FREE_SLOT; \
		*FIRST_FREE_SLOT = (*FIRST_FREE_SLOT)->NEXT; \
	} else { \
		*(void**)OUT_SLOT = DS_BkArrPushZero(BUCKET_ARRAY); \
	}

// Free a slot from a bucket array with a pointer-based freelist
#define FREE_SLOT_PTR(SLOT, FIRST_FREE_INDEX, NEXT) \
	SLOT->NEXT = *FIRST_FREE_INDEX; \
	*FIRST_FREE_INDEX = SLOT;

// -- Globals ---------------------------------------------------------

extern DS_Arena* TEMP;
extern DS_MemScope MEM_SCOPE_TEMP_;
extern DS_MemScopeNone MEM_SCOPE_NONE_;
extern uint64_t CPU_FREQUENCY;
extern STR_View DEFAULT_WORKING_DIRECTORY;

#define MEM_SCOPE_NONE   (DS_MemScopeNone*)&MEM_SCOPE_NONE_
#define MEM_SCOPE_TEMP   (DS_MemScope*)&MEM_SCOPE_TEMP_
#define MEM_SCOPE(ARENA) DS_MemScope{ ARENA, TEMP }

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

union DecodedHandle {
	struct {
		u16 bucket_index;
		u16 elem_index;
		u32 generation;
	};
	u64 val;
};

static inline void* EncodeHandle(DecodedHandle handle) { return *(void**)&handle; }
static inline DecodedHandle DecodeHandle(void* handle) { return *(DecodedHandle*)&handle; }

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

struct StructMember {
	HT_String name;
	HT_Type type;
	i32 offset;
};

struct Asset_StructType {
	DS_DynArray(StructMember) members;
	i32 size;
	i32 alignment;
	HT_PluginInstance asset_viewer_registered_by_plugin;
	TabUpdateProc asset_viewer_update_proc;
	//DS_Set(Asset*) uses_struct_types; // recursively contains all struct types this contains
};

struct Asset_Package {
	STR_View filesys_path;
	OS_DirectoryWatch dir_watch;
};

struct PluginOptions {
	HT_Array code_files; // Array<AssetRef>
	HT_Asset data;
};

struct PluginAllocationHeader {
	u32 allocation_index;
	size_t size;
};

struct Asset_Plugin {
	PluginOptions options; // a value of type g_plugin_options_struct_type
	
	bool active_by_request;
	HT_PluginInstance active_instance;
};

struct Asset_StructData {
	HT_Asset struct_type;

	// When editing a type, just loop through all structs of that type and update the data layout.
	// ... When changing a member type in the struct type, removing a member, or destroying the struct type asset, that is a destructive change.
	//     But we don't want to destroy the data. TODO: we should have an additional array of "stale_members" which stores the name, type and value of old members.
	//     The stale array could then be serialized as regular.
	void* data;
};

struct Asset {
	AssetKind kind;
	HT_String name; // not used for AssetKind_Package
	HT_Asset handle;
	u64 modtime;

	Asset* parent;
	Asset* prev;
	Asset* next;
	Asset* first_child;
	Asset* last_child;

	union {
		DS_BucketArrayIndex freelist_next;
		Asset_StructType struct_type;
		Asset_StructData struct_data;
		Asset_Plugin plugin;
		Asset_Package package;
	};

	STR_View reload_assets_filesys_path; // temporary variable
	bool reload_assets_pass2_needs_hotreload; // temporary variable

	bool ui_state_is_open; // for the Assets panel
};

struct AssetTree {
	DS_Map(u64, Asset*) package_from_name; // key is the DS_MurmurHash64A(0) of the package name (excluding the $)

	DS_BucketArray(Asset) assets;
	DS_BucketArrayIndex first_free_asset;

	Asset* root;

	// built-in types
	Asset* plugin_options_struct_type;  // built-in custom struct type for PluginOptions
	Asset* name_and_type_struct_type;   // built-in custom struct type for NameAndType
};

// Pass STR_View into printf-style %.*s arguments
#define StrArg(X) (int)X.size, X.data

EXPORT STR_View HT_TypeKindToString(HT_TypeKind type);
EXPORT HT_TypeKind StringToTypeKind(STR_View str); // may return HT_TypeKind_INVALID

EXPORT Asset* GetAsset(AssetTree* tree, HT_Asset handle); // returns NULL if the handle is invalid

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

EXPORT void AnyChangeType(AssetTree* tree, HT_Any* any, HT_Type* new_type);
EXPORT void AnyDeinit(AssetTree* tree, HT_Any* any);

EXPORT void ArrayPush(HT_Array* array, i32 elem_size);
EXPORT void ArrayClear(HT_Array* array, i32 elem_size);
EXPORT void ArrayDeinit(HT_Array* array);

EXPORT void ItemGroupInit(AssetTree* tree, HT_ItemGroup* group, HT_Type* item_type);
EXPORT void ItemGroupDeinit(HT_ItemGroup* group);
EXPORT HT_ItemIndex ItemGroupAdd(HT_ItemGroup* group); // does not insert the asset into the list yet, you must call MoveItemToAfter
EXPORT void ItemGroupRemove(HT_ItemGroup* group, HT_ItemIndex item);

// `item` may be HT_ITEM_INDEX_INVALID, in which case NULL is returned. Otherwise the index must be valid.
EXPORT HT_ItemHeader* GetItemFromIndex(HT_ItemGroup* group, HT_ItemIndex item);

// if `move_after_this` is HT_ITEM_INDEX_INVALID, the item is moved to the beginning of the list.
EXPORT void MoveItemToAfter(HT_ItemGroup* group, HT_ItemIndex item, HT_ItemIndex move_after_this);

EXPORT void StructMemberInit(StructMember* x); // value must be zero-initialized already
EXPORT void StructMemberDeinit(StructMember* x);
EXPORT void StringInit(HT_String* x, STR_View value); // `x` may be uninitialized before
EXPORT void StringDeinit(HT_String* x);
EXPORT void StringSetValue(HT_String* x, STR_View value);

// - returns NULL if not found
EXPORT Asset* FindAssetFromPath(AssetTree* tree, Asset* package, STR_View path);

// -- ui.cpp ----------------------------------------------------------

struct UI_Tab; // Placeholder for the user

struct UI_Panel {
	bool is_alive; // todo: cleanup
	union {
		UI_Panel* parent;
		UI_Panel* freelist_next;
	};
	UI_Panel* end_child[2]; // 0 is first, 1 is last
	UI_Panel* link[2];      // 0 is prev, 1 is next

	float size;
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
	UI_Key deepest_hovered_root;
	UI_Key deepest_hovered_root_new;
	bool has_added_deepest_hovered_root;
};

EXPORT void UI_AddDropdownButton(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string, UI_Font icons_font);

EXPORT void UI_PanelTreeInit(UI_PanelTree* tree, DS_Allocator* allocator);
EXPORT void UI_PanelTreeUpdateAndDraw(UIDropdownState* s, UI_PanelTree* tree, UI_Panel* panel, UI_Rect area_rect, bool splitter_is_hovered, UI_Font icons_font, UI_Panel** out_hovered);

EXPORT UI_Panel* NewUIPanel(UI_PanelTree* tree);
EXPORT void FreeUIPanel(UI_PanelTree* tree, UI_Panel* panel);

EXPORT HT_String UITextToString(UI_Text text);
EXPORT UI_Text StringToUIText(HT_String string);

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

enum LogMessageKind {
	LogMessageKind_Info,
	LogMessageKind_Warning,
	LogMessageKind_Error,
};

struct LogMessage {
	LogMessageKind kind;
	STR_View string;
	uint64_t added_tick;
};

struct Log {
	DS_Arena arena;
	DS_DynArray(LogMessage) messages;
};

struct Error {
	// for now, theres just compile errors
	HT_Asset owner_asset;
	STR_View string;
	uint64_t added_tick;
};

// Tracks errors in real time. A single plugin that fails to compile should probably be just one error in the error list,
// or possibly a collapsible item that can then show more errors when opened.
struct ErrorList {
	DS_DynArray(Error) errors;
};

struct PluginInstance {
	HT_PluginInstance handle;
	Asset* plugin_asset; // NULL if unloaded

	union {
		DS_BucketArrayIndex freelist_next;
		OS_DLL* dll_handle;
	};

	void (*UpdatePlugin)(HT_API* HT);
	void (*LoadPlugin)(HT_API* HT);
	void (*UnloadPlugin)(HT_API* HT);

#ifdef HT_EDITOR_DX11
	void (*HT_D3D11_Render)(HT_API* ht);
#endif

	DS_DynArray(PluginAllocationHeader*) allocations;
};

// TODO: special tab data for per-tab data like which asset an asset viewer is looking at
struct UI_Tab {
	STR_View name; // empty string means free slot
	HT_Asset owner_plugin;
	UI_Tab* freelist_next;
};

struct PerFrameState {
	UI_Panel* hovered_panel;
	bool file_dropdown_open;
	bool edit_dropdown_open;

	UI_Box* file_dropdown;
	UI_Box* file_dropdown_button;

	UI_Box* window_dropdown;
	UI_Box* window_dropdown_button;
	
	UI_Box* type_dropdown;
	UI_Box* type_dropdown_button;

	DS_DynArray(HT_CustomTabUpdate) queued_custom_tab_updates;
};

struct EditorState {
	HT_API* api;

	DS_Arena persistent_arena;
	
	ivec2 window_size;
	OS_Window window;

	HT_InputFrame input_frame;
	
	UI_Inputs ui_inputs;
	UI_Font default_font;
	UI_Font icons_font;

	STR_View project_directory;

	DS_BucketArray(PluginInstance) plugin_instances;
	DS_BucketArrayIndex first_free_plugin_instance;

	Log log;

	ErrorList error_list;

	AssetTree asset_tree;
	UI_DataTreeState assets_tree_ui_state;
	DS_DynArray(HT_Asset) type_table; // quick and dirty solution

	UI_DataTreeState properties_tree_type_ui_state;
	UI_DataTreeState properties_tree_data_ui_state;

	UIDropdownState dropdown_state;

	DS_BucketArray(UI_Tab) tab_classes;
	UI_Tab* first_free_tab_class;
	
	UI_Tab* properties_tab_class;
	UI_Tab* assets_tab_class;
	UI_Tab* asset_viewer_tab_class;
	UI_Tab* log_tab_class;
	UI_Tab* errors_tab_class;
	
	UI_PanelTree panel_tree;

	// -- simulation state --

	bool is_simulating;
	bool pending_reload_packages; // set to true after pressing "Stop" - delay it until the start of the next frame
	
	// ------------------
	
	vec2 rmb_menu_pos;
	UI_Tab* rmb_menu_tab_class;
	bool rmb_menu_open;

	bool file_dropdown_open;
	bool window_dropdown_open;
	UI_Key type_dropdown_open;
	
	// ------------------

	RenderState* render_state;

	PerFrameState frame; // cleared at the beginning of a frame
};

EXPORT void RunPlugin(EditorState* s, Asset* plugin);
EXPORT void UnloadPlugin(EditorState* s, Asset* plugin);

EXPORT UI_Tab* CreateTabClass(EditorState* s, STR_View name);
EXPORT void DestroyTabClass(EditorState* s, UI_Tab* tab);

EXPORT void UpdateAndDrawTab(UI_PanelTree* tree, UI_Tab* tab, UI_Key key, UI_Rect area_rect);

EXPORT void AddTopBar(EditorState* s);
EXPORT void UpdateAndDrawAssetsBrowserTab(EditorState* s, UI_Key key, UI_Rect area);
EXPORT void UpdateAndDrawPropertiesTab(EditorState* s, UI_Key key, UI_Rect area);
EXPORT void UpdateAndDrawLogTab(EditorState* s, UI_Key key, UI_Rect area);
EXPORT void UpdateAndDrawErrorsTab(EditorState* s, UI_Key key, UI_Rect area);

EXPORT void InitAPI(EditorState* s);

EXPORT void UpdatePlugins(EditorState* s);

#ifdef HT_EDITOR_DX12
EXPORT void D3D12_BuildPluginCommandLists(EditorState* s);
#endif

#ifdef HT_EDITOR_DX11
EXPORT void D3D11_RenderPlugins(EditorState* s);
#endif

EXPORT void UpdateAndDrawDropdowns(EditorState* s);

EXPORT PluginInstance* GetPluginInstance(EditorState* s, HT_PluginInstance handle); // returns NULL if the handle is invalid

EXPORT void RemoveErrorsByAsset(ErrorList* error_list, HT_Asset asset);

// -- ht_serialize.cpp ------------------------------------------------

EXPORT STR_View AssetGetPackageRelativePath(DS_Arena* arena, Asset* asset);
EXPORT STR_View AssetGetAbsoluteFilepath(DS_Arena* arena, Asset* asset);

EXPORT void HotreloadPackages(AssetTree* tree);

EXPORT STR_View GetAssetFileExtension(Asset* asset);

EXPORT void SavePackageToDisk(Asset* package);

EXPORT void RegenerateTypeTable(EditorState* s);

EXPORT void LoadProject(AssetTree* tree, STR_View project_directory);
EXPORT void LoadProjectIncludingEditorLayout(EditorState* s, STR_View project_directory);
EXPORT void ReloadPackages(AssetTree* tree, DS_ArrayView<Asset*> packages, bool force_reload);
EXPORT void LoadPackages(AssetTree* tree, DS_ArrayView<STR_View> paths);

// -- ht_log.cpp ------------------------------------------------------

EXPORT void LogF(Log* log, LogMessageKind kind, const char* fmt, ...);
EXPORT void LogVArgs(Log* log, LogMessageKind kind, const char* fmt, va_list args);

EXPORT void UpdateAndDrawLogTab(EditorState* s, UI_Key key, UI_Rect area);

// -- ht_plugin_compiler.cpp ------------------------------------------

EXPORT void GeneratePremakeAndVSProjects(AssetTree* asset_tree, STR_View project_directory);

EXPORT void RegeneratePluginHeader(AssetTree* tree, Asset* plugin);

#ifdef HT_DYNAMIC
EXPORT void ForceVisualStudioToClosePDBFileHandle(STR_View pdb_filepath);

EXPORT bool RecompilePlugin(EditorState* s, Asset* plugin);
#endif

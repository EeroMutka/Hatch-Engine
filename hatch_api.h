// This header acts as the common interface between Hatch and any plugins that call into Hatch.

#pragma once
#define HATCH_API_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <immintrin.h>
#include <intrin.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;
typedef uint64_t  u64;
typedef int8_t    i8;
typedef int16_t   i16;
typedef int32_t   i32;
typedef int64_t   i64;

#ifdef __cplusplus
#define HT_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define HT_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

// NOTE: may be omitted in debug builds!
#define HT_ASSERT(X) do { if (!(X)) __debugbreak(); } while(0)

typedef union vec2 {
	struct { float x, y; };
	float _[2];
} vec2;

typedef union vec3 {
	struct { float x, y, z; };
	struct { vec2 xy; float z; };
	struct { float x; vec2 yz; };
	float _[3];
} vec3;

typedef union vec4 {
	struct { vec3 xyz; float w; };
	struct { float x; vec3 yzw; };
	struct { vec2 xy; vec2 zw; };
	struct { float x; vec2 yz; float w; };
	struct { float x, y, z, w; };
	float _[4];
	__m128 SSE;
//#ifdef __cplusplus
//	vec4(float _x, float _y, float _z, float _w) {}
//	vec4(vec2 xy, vec2 zw) {}
//	vec4(float x, vec2 yz, float w) {}
//	vec4(vec3 xyz, float w) {}
//	vec4(float x, vec3 yzw) {}
//#endif
} vec4;

typedef union ivec2 {
	struct { int x, y; };
	int _[2];
} ivec2;

typedef union ivec3 {
	struct { int x, y, z; };
	struct { ivec2 xy; int z; };
	struct { int x; ivec2 yz; };
	int _[3];
} ivec3;

typedef union ivec4 {
	struct { int x, y, z, w; };
	struct { ivec2 xy; ivec2 zw; };
	struct { int x; ivec2 yz; int w; };
	struct { ivec3 xyz; int w; };
	struct { int x; ivec3 yzw; };
	int _[4];
} ivec4;

typedef struct HT_StringView { // utf8-encoded string view, not null-terminated
	char* data;
	size_t size;
#ifdef __cplusplus
	inline HT_StringView() : data(0), size(0) {}
	inline HT_StringView(const char* _data, size_t _size) : data((char*)_data), size(_size) {}
	inline HT_StringView(const char* _cstr) : data((char*)_cstr), size(_cstr ? strlen(_cstr) : 0) {}
	inline HT_StringView(struct HT_String string); // needs to be defined after the HT_String struct
#endif
} HT_StringView;

typedef struct HT_String { // utf8-encoded string, not null-terminated. TODO: we might as well include a null-terminator?
	union {
		struct { char* data; size_t size; };
		HT_StringView view;
	};
	size_t capacity;
} HT_String;

#ifdef __cplusplus
inline HT_StringView::HT_StringView(HT_String string) : data(string.data), size(string.size) {}
#endif

typedef enum HT_TypeKind {
	HT_TypeKind_Float,
	HT_TypeKind_Int,
	HT_TypeKind_Bool,
	HT_TypeKind_Vec2,
	HT_TypeKind_Vec3,
	HT_TypeKind_Vec4,
	HT_TypeKind_IVec2,
	HT_TypeKind_IVec3,
	HT_TypeKind_IVec4,
	HT_TypeKind_AssetRef,
	HT_TypeKind_Struct,
	HT_TypeKind_Array,
	HT_TypeKind_Any,
	HT_TypeKind_ItemGroup,
	HT_TypeKind_String,
	HT_TypeKind_Type,
	HT_TypeKind_COUNT,
	HT_TypeKind_INVALID,
} HT_TypeKind;

typedef struct HT_Asset_* HT_Asset; // handle to an asset
typedef struct HT_PluginInstance_* HT_PluginInstance; // handle to a plugin instance

typedef struct HT_Array {
	void* data;
	i32 count;
	i32 capacity;
} HT_Array;

// HT_ItemIndex encodes the following struct: { u16 bucket_index_plus_one; slot_index; }
// 0 is always invalid.
typedef u32 HT_ItemIndex;

#define HT_MakeItemIndex(BUCKET, ELEM)  (u32)(((u32)(BUCKET) + 1) | ((ELEM) << 16))
#define HT_ItemIndexBucket(INDEX)       ((u16)(INDEX) - 1)
#define HT_ItemIndexElem(INDEX)         (u16)((INDEX) >> 16)

#define HT_GetItemHeader(ITEM_GROUP, INDEX) ((HT_ItemHeader*)(((char**)(ITEM_GROUP)->buckets.data)[HT_ItemIndexBucket(INDEX)] + HT_ItemIndexElem(INDEX)*(ITEM_GROUP)->item_full_size))
#define HT_GetItem(T, ITEM_GROUP, INDEX) (T*)((char*)HT_GetItemHeader(ITEM_GROUP, INDEX) + (ITEM_GROUP)->item_offset)
#define HT_NextItem(ITEM_GROUP, INDEX) HT_GetItemHeader(ITEM_GROUP, INDEX)->next
#define HT_ItemGroupEach(ITEM_GROUP, IT) HT_ItemIndex IT = (ITEM_GROUP)->first; IT; IT = HT_NextItem(ITEM_GROUP, IT)

// Each item is part of a doubly linked list, so everything is always ordered.
typedef struct HT_ItemGroup {
	HT_Array buckets;
	//i32 count;
	i32 last_bucket_end;
	i32 elems_per_bucket;
	i32 item_offset;
	i32 item_full_size;
	HT_ItemIndex first;
	HT_ItemIndex last;
	HT_ItemIndex freelist_first;
} HT_ItemGroup;

typedef struct HT_ItemHeader {
	HT_ItemIndex prev;
	HT_ItemIndex next;
	HT_String name;
} HT_ItemHeader;

typedef struct HT_ItemHandleDecoded {
	HT_ItemIndex index;
	u32 generation;
} HT_ItemHandleDecoded;

// Not actually a pointer - encodes a HT_ItemHandleDecoded structure. Typedef'd to a pointer for type safety and easy comparison.
typedef struct HT_ItemHandle_* HT_ItemHandle;

typedef struct HT_Type {
	HT_TypeKind kind;
	HT_TypeKind subkind; // for arrays, this is the element type
	HT_Asset handle; // Handle to the struct asset if struct, otherwise null.
} HT_Type;

typedef struct HT_Any {
	HT_Type type;
	void* data;
} HT_Any;

typedef struct HT_Color {
	u8 r, g, b, a;
} HT_Color;
#define HT_COLOR HT_LangAgnosticLiteral(HT_Color)

typedef struct HT_DrawVertex {
	vec2 position;
	vec2 uv; // UV into the texture atlas. At {0, 0} there is a white pixel in the atlas for easy untextured triangles
	HT_Color color;
} HT_DrawVertex;
#define HT_DRAW_VERTEX HT_LangAgnosticLiteral(HT_DrawVertex)

typedef struct HT_CachedGlyph {
	vec2 uv_min; // in normalized UV coordinates
	vec2 uv_max; // in normalized UV coordinates
	vec2 offset; // in pixel coordinates
	vec2 size;   // in pixel coordinates
	float advance; // X-advance to the next character in pixel coordinates
} HT_CachedGlyph;

typedef struct HT_StaticExports {
	const char* plugin_id;
	void (*HT_LoadPlugin)(struct HT_API* ht);
	void (*HT_UnloadPlugin)(struct HT_API* ht);
	void (*HT_UpdatePlugin)(struct HT_API* ht);
} HT_StaticExports;

#ifdef HT_STATIC_PLUGIN_ID
#ifdef __cplusplus
extern "C" {
#endif

static void HT_LoadPlugin(struct HT_API* ht);
static void HT_UnloadPlugin(struct HT_API* ht);
static void HT_UpdatePlugin(struct HT_API* ht);

#define HT__Concat2(a, b) a ## b
#define HT__Concat(a, b) HT__Concat2(a, b)
#define HT__Stringify2(x) #x
#define HT__Stringify(x) HT__Stringify2(x)

// The HT_STATIC_PLUGIN_ID must be the same as the plugin name.
HT_StaticExports HT__Concat(HT_STATIC_EXPORTS__, HT_STATIC_PLUGIN_ID) = { HT__Stringify(HT_STATIC_PLUGIN_ID), HT_LoadPlugin, HT_UnloadPlugin, HT_UpdatePlugin };

#ifdef __cplusplus
} // extern "C"
#endif
#else
#ifndef HT_NO_STATIC_PLUGIN_EXPORTS
#error HT_STATIC_PLUGIN_ID must be defined before including <hatch_api.h>
#endif
#endif

#ifndef HT_EXPORT
#ifdef __cplusplus
#define HT_EXPORT extern "C" __declspec(dllexport)
#else
#define HT_EXPORT __declspec(dllexport)
#endif
#endif

#ifndef HT_IMPORT
#ifdef __cplusplus
#define HT_IMPORT extern "C" __declspec(dllimport)
#else
#define HT_IMPORT __declspec(dllimport)
#endif
#endif

typedef struct HT_GeneratedTypeTable HT_GeneratedTypeTable;

typedef struct HT_Rect {
	vec2 min;
	vec2 max;
} HT_Rect;

typedef struct HT_AssetViewerTabUpdate {
	HT_Asset data_asset;
	HT_Rect rect;

	// drag-n-drop
	HT_Asset drag_n_dropped_asset; // NULL if none
} HT_AssetViewerTabUpdate;

typedef void (*TabUpdateProc)(struct HT_API* ht, const HT_AssetViewerTabUpdate* update_info);

typedef struct HT_TabClass HT_TabClass;

typedef struct HT_CustomTabUpdate {
	HT_TabClass* tab_class;
	HT_Rect rect;
} HT_CustomTabUpdate;

typedef enum HT_InputKey {
	HT_InputKey_Invalid = 0,

	HT_InputKey_Space = 32,
	HT_InputKey_Apostrophe = 39,   /* ' */
	HT_InputKey_Comma = 44,        /* , */
	HT_InputKey_Minus = 45,        /* - */
	HT_InputKey_Period = 46,       /* . */
	HT_InputKey_Slash = 47,        /* / */

	HT_InputKey_0 = 48,
	HT_InputKey_1 = 49,
	HT_InputKey_2 = 50,
	HT_InputKey_3 = 51,
	HT_InputKey_4 = 52,
	HT_InputKey_5 = 53,
	HT_InputKey_6 = 54,
	HT_InputKey_7 = 55,
	HT_InputKey_8 = 56,
	HT_InputKey_9 = 57,

	HT_InputKey_Semicolon = 59,    /* ; */
	HT_InputKey_Equal = 61,        /* = */
	HT_InputKey_LeftBracket = 91,  /* [ */
	HT_InputKey_Backslash = 92,    /* \ */
	HT_InputKey_RightBracket = 93, /* ] */
	HT_InputKey_GraveAccent = 96,  /* ` */

	HT_InputKey_A = 65,
	HT_InputKey_B = 66,
	HT_InputKey_C = 67,
	HT_InputKey_D = 68,
	HT_InputKey_E = 69,
	HT_InputKey_F = 70,
	HT_InputKey_G = 71,
	HT_InputKey_H = 72,
	HT_InputKey_I = 73,
	HT_InputKey_J = 74,
	HT_InputKey_K = 75,
	HT_InputKey_L = 76,
	HT_InputKey_M = 77,
	HT_InputKey_N = 78,
	HT_InputKey_O = 79,
	HT_InputKey_P = 80,
	HT_InputKey_Q = 81,
	HT_InputKey_R = 82,
	HT_InputKey_S = 83,
	HT_InputKey_T = 84,
	HT_InputKey_U = 85,
	HT_InputKey_V = 86,
	HT_InputKey_W = 87,
	HT_InputKey_X = 88,
	HT_InputKey_Y = 89,
	HT_InputKey_Z = 90,

	HT_InputKey_Escape = 256,
	HT_InputKey_Enter = 257,
	HT_InputKey_Tab = 258,
	HT_InputKey_Backspace = 259,
	HT_InputKey_Insert = 260,
	HT_InputKey_Delete = 261,
	HT_InputKey_Right = 262,
	HT_InputKey_Left = 263,
	HT_InputKey_Down = 264,
	HT_InputKey_Up = 265,
	HT_InputKey_PageUp = 266,
	HT_InputKey_PageDown = 267,
	HT_InputKey_Home = 268,
	HT_InputKey_End = 269,
	HT_InputKey_CapsLock = 280,
	HT_InputKey_ScrollLock = 281,
	HT_InputKey_NumLock = 282,
	HT_InputKey_PrintScreen = 283,
	HT_InputKey_Pause = 284,

	HT_InputKey_F1 = 290,
	HT_InputKey_F2 = 291,
	HT_InputKey_F3 = 292,
	HT_InputKey_F4 = 293,
	HT_InputKey_F5 = 294,
	HT_InputKey_F6 = 295,
	HT_InputKey_F7 = 296,
	HT_InputKey_F8 = 297,
	HT_InputKey_F9 = 298,
	HT_InputKey_F10 = 299,
	HT_InputKey_F11 = 300,
	HT_InputKey_F12 = 301,
	HT_InputKey_F13 = 302,
	HT_InputKey_F14 = 303,
	HT_InputKey_F15 = 304,
	HT_InputKey_F16 = 305,
	HT_InputKey_F17 = 306,
	HT_InputKey_F18 = 307,
	HT_InputKey_F19 = 308,
	HT_InputKey_F20 = 309,
	HT_InputKey_F21 = 310,
	HT_InputKey_F22 = 311,
	HT_InputKey_F23 = 312,
	HT_InputKey_F24 = 313,
	HT_InputKey_F25 = 314,

	HT_InputKey_KP_0 = 320,
	HT_InputKey_KP_1 = 321,
	HT_InputKey_KP_2 = 322,
	HT_InputKey_KP_3 = 323,
	HT_InputKey_KP_4 = 324,
	HT_InputKey_KP_5 = 325,
	HT_InputKey_KP_6 = 326,
	HT_InputKey_KP_7 = 327,
	HT_InputKey_KP_8 = 328,
	HT_InputKey_KP_9 = 329,

	HT_InputKey_KP_Decimal = 330,
	HT_InputKey_KP_Divide = 331,
	HT_InputKey_KP_Multiply = 332,
	HT_InputKey_KP_Subtract = 333,
	HT_InputKey_KP_Add = 334,
	HT_InputKey_KP_Enter = 335,
	HT_InputKey_KP_Equal = 336,

	HT_InputKey_LeftShift = 340,
	HT_InputKey_LeftControl = 341,
	HT_InputKey_LeftAlt = 342,
	HT_InputKey_LeftSuper = 343,
	HT_InputKey_RightShift = 344,
	HT_InputKey_RightControl = 345,
	HT_InputKey_RightAlt = 346,
	HT_InputKey_RightSuper = 347,
	HT_InputKey_Menu = 348,

	// Events for these four modifier keys shouldn't be generated, nor should they be used in the `key_is_down` table.
	// They're purely for convenience when calling the HT_InputIsDown/WentDown/WentDownOrRepeat/WentUp functions.
	HT_InputKey_Shift = 349,
	HT_InputKey_Control = 350,
	HT_InputKey_Alt = 351,
	HT_InputKey_Super = 352,

	HT_InputKey_MouseLeft = 353,
	HT_InputKey_MouseRight = 354,
	HT_InputKey_MouseMiddle = 355,
	HT_InputKey_Mouse_4 = 356,
	HT_InputKey_Mouse_5 = 357,
	HT_InputKey_Mouse_6 = 358,
	HT_InputKey_Mouse_7 = 359,
	HT_InputKey_Mouse_8 = 360,

	HT_InputKey_COUNT,
} HT_InputKey;

typedef uint8_t HT_InputEventKind;
enum {
	HT_InputEventKind_Press,
	HT_InputEventKind_Repeat,
	HT_InputEventKind_Release,
	HT_InputEventKind_TextCharacter,
};

typedef struct HT_InputEvent {
	HT_InputEventKind kind;
	uint8_t mouse_click_index; // 0 for regular click, 1 for double-click, 2 for triple-click
	union {
		HT_InputKey key; // for Press, Repeat, Release events
		u32 text_character; // unicode character for TextCharacter event
	};
} HT_InputEvent;

typedef struct HT_InputFrame {
	HT_InputEvent* events;
	int events_count;
	bool key_is_down[HT_InputKey_COUNT]; // This is the key down state after the events for this frame have been applied
	float mouse_wheel_input[2]; // +1.0 means the wheel was rotated forward by one detent (scroll step)
	float raw_mouse_input[2];
	vec2 mouse_position;
} HT_InputFrame;

// Helpers
// #define HT_GetPluginData(TYPE, HT) (TYPE*)HT->GetPluginData(HT->type_table->TYPE)
#define HT_GetAssetData(TYPE, HT, ASSET) (TYPE*)HT->AssetGetData(ASSET)
#define HT_GetPluginData(TYPE, HT) (TYPE*)HT->GetPluginData()

struct HT_API {
	// Exportable functions from a plugin:
	// HT_EXPORT void HT_LoadPlugin(HT_API* ht)
	// HT_EXPORT void HT_UnloadPlugin(HT_API* ht)
	// HT_EXPORT void HT_UpdatePlugin(HT_API* ht) 
	
	// So, maybe we want to keep the plugin API minimal. Because that is *required* for everyone to use. If we had e.g. string utilities separately, then the user could copy
	// the string utility header into their own plugin folder and keep using an older version if they wanted to.
	// The best solution I can think of is to do semver on utility libraries and to ship all major versions as separate folders as part of hatch. Then you do
	// #include "ht_utilities/string/v0_experimental/string.h"
	// #include "ht_utilities/string/v1/string.h"
	// #include "ht_utilities/string/v2/string.h"
	
	// The plugin API should be kept minimal / direct access only. This is to make it more stable and to allow for rapid iteration elsewhere.
	// And also to reduce dynamic dispatch in performance critical tasks.
	// For example, Arenas could be implemented on the user / utility side.
	// The API could provide malloc/free.
	
	// RESTRICTIONS:
	// I think we should artificially limit the plugin to only drawing in special plugin-defined tabs.
	// There should be sane default restrictions to make sure the user of Hatch always has a sane time and won't lose control.
	// The user should always be in control.
	
	// -- Data model ----------------------------------
	
	const HT_GeneratedTypeTable* types;
	
	// Returns NULL if data asset is invalid or of different type than `type_id`
	void* (*GetPluginData)(/*HT_Asset type_id*/);

	// Works on data assets. Returns NULL if asset ref is invalid or not a data asset.
	void* (*AssetGetData)(HT_Asset asset);

	// Works on data assets. Returns NULL if asset ref is invalid or not a data asset.
	HT_Asset (*AssetGetType)(HT_Asset asset);

	// bool (*AssetIsValid)(/*HT_Asset type_id, */HT_Asset asset);
	 
	// Returns an empty string if the asset is invalid, otherwise an absolute filepath to the asset.
	// The returned string is valid for this frame only.
	HT_StringView (*AssetGetFilepath)(HT_Asset asset);
	
	// Whenever an asset data is changed, the modtime integer becomes larger.
	// For folders, the modtime is always the maximum modtime of any asset inside it (applies recursively).
	// Returns 0 if the asset ref is invalid.
	u64 (*AssetGetModtime)(HT_Asset asset);

	// Item group utilities
	HT_ItemIndex (*ItemGroupAdd)(HT_ItemGroup* group);
	void (*ItemGroupRemove)(HT_ItemGroup* group, HT_ItemIndex item);
	void (*MoveItemToAfter)(HT_ItemGroup* group, HT_ItemIndex item, HT_ItemIndex move_after_this);
	
	// -- Plugins -------------------------------------
	
	// NOTE: A plugin may be unloaded in-between frames, never in the middle of a frame.
	HT_PluginInstance (*GetPluginInstance)(HT_Asset plugin_asset); // returns NULL if plugin is not active
	bool (*PluginInstanceIsValid)(HT_PluginInstance plugin_instance);
	void* (*PluginInstanceFindProcAddress)(HT_PluginInstance plugin_instance, HT_StringView name); // returns NULL if not found
	
	// -- Memory allocation ---------------------------
	
	// AllocatorProc is a combination of malloc, free and realloc.
	// A new allocation is made (or an existing allocation is resized if ptr != NULL) when size > 0.
	// An existing allocation is freed when size == 0.
	// All allocations are aligned to 16 bytes.
	void* (*AllocatorProc)(void* ptr, size_t size);
	
	// The returned memory from TempArenaPush gets automatically free'd after the current frame.
	// The returned memory is uninitialized.
	void* (*TempArenaPush)(size_t size, size_t align);
	
	// -- Asset viewer -------------------------------
	
	// Returns the selected item handle in the properties panel for the selected asset or NULL if none
	HT_ItemHandle (*GetSelectedItemHandle)();

	bool (*RegisterAssetViewerForType)(HT_Asset struct_type_asset, TabUpdateProc update_proc);
	void (*UnregisterAssetViewerForType)(HT_Asset struct_type_asset);
	
	// Returns a temporary array (i.e. TempArenaPush)
	HT_Asset* (*GetAllOpenAssetsOfType)(HT_Asset struct_type_asset, int* out_count);
	
	// -- Simulation ---------------------------------

	bool (*IsSimulating)();

	// -- Input --------------------------------------
	
	const HT_InputFrame* input_frame;
	
	// -- UI -----------------------------------------
	
	HT_TabClass* (*CreateTabClass)(HT_StringView name);
	void (*DestroyTabClass)(HT_TabClass* tab);
	
	// poll next custom tab update
	bool (*PollNextCustomTabUpdate)(HT_CustomTabUpdate* tab_update);
	
	// Returns the index of the first new vertex
	u32 (*AddVertices)(HT_DrawVertex* vertices, int count);
	
	void (*AddIndices)(u32* indices, int count);
	
	// Maybe we can still have a basic 2D drawing lib built into the API.
	// Like draw text, draw circle, draw rect, etc.
	// void (*DrawText)(string text, vec2 pos, HT_AlignH align, int font_size, HT_Color color);
	
	HT_CachedGlyph (*GetCachedGlyph)(u32 codepoint);
	
	// -- OS -----------------------------------------

	void* (*GetOSWindowHandle)(); // returns HWND on Windows
	
	// -- D3D11 API -----------------------------------
	
#ifdef HT_INCLUDE_D3D11_API
	ID3D11Device* D3D11_device;
	ID3D11DeviceContext* D3D11_device_context;
	
	void (*D3D11_SetRenderProc)(void (*render)(HT_API* ht));

	HRESULT (*D3D11_Compile)(const void* pSrcData, size_t SrcDataSize, const char* pSourceName,
		const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude, const char* pEntrypoint,
		const char* pTarget, u32 Flags1, u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);
	
	// NOTE: The filename parameter is wchar_t* in the official D3DCompiler API, but Hatch does a conversion here.
	HRESULT (*D3D11_CompileFromFile)(HT_StringView FileName, const D3D_SHADER_MACRO* pDefines,
		ID3DInclude* pInclude, const char* pEntrypoint, const char* pTarget, u32 Flags1,
		u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);
	
	ID3D11RenderTargetView* (*D3D11_GetHatchRenderTargetView)();
#endif
	
	// -- D3D12 API -----------------------------------
	
#ifdef HT_INCLUDE_D3D12_API
	ID3D12Device* D3D_device;
	ID3D12CommandQueue* D3D_queue;
	
	// When using D3D12, you may define the function:
	// HT_EXPORT void HT_D3D12_BuildPluginCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list)
	
	HRESULT (*D3DCompile)(const void* pSrcData, size_t SrcDataSize, const char* pSourceName,
		const D3D_SHADER_MACRO* pDefines, ID3DInclude* pInclude, const char* pEntrypoint,
		const char* pTarget, u32 Flags1, u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);
	
	// NOTE: The filename parameter is wchar_t* in the official D3DCompiler API, but Hatch does a conversion here.
	HRESULT (*D3DCompileFromFile)(string FileName, const D3D_SHADER_MACRO* pDefines,
		ID3DInclude* pInclude, const char* pEntrypoint, const char* pTarget, u32 Flags1,
		u32 Flags2, ID3DBlob** ppCode, ID3DBlob** ppErrorMsgs);
	
	HRESULT (*D3D12SerializeRootSignature)(const D3D12_ROOT_SIGNATURE_DESC* pRootSignature,
		D3D_ROOT_SIGNATURE_VERSION Version, ID3DBlob** ppBlob, ID3DBlob** ppErrorBlob);
	
	D3D12_CPU_DESCRIPTOR_HANDLE (*D3DGetHatchRenderTargetView)();
	
	HANDLE (*D3DCreateEvent)(); // Equivelent to `CreateEventW(NULL, FALSE, FALSE, NULL)`
	void (*D3DDestroyEvent)(HANDLE event); // Equivelent to `CloseHandle(event)`
	void (*D3DWaitForEvent)(HANDLE event); // Equivelent to `WaitForSingleObjectEx(event, INFINITE, FALSE)`
#endif
	
	// ------------------------------------------------
};

// Hatch debug API - the idea is that you can access this from anywhere without requiring access to the HT_API.

HT_IMPORT void HT_LogInfo(const char* fmt, ...);
HT_IMPORT void HT_LogWarning(const char* fmt, ...);
HT_IMPORT void HT_LogError(const char* fmt, ...);


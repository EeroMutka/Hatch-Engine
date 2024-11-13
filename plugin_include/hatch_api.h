// This header acts as the common interface between Hatch and any plugins that call into Hatch.

#pragma once
#define HATCH_API_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

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
	struct { float x, y, z, w; };
	struct { vec2 xy; vec2 zw; };
	struct { float x; vec2 yz; float w; };
	struct { vec3 xyz; float w; };
	struct { float x; vec3 yzw; };
	float _[4];
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

typedef struct string { // utf8-encoded string
	char* data;
	int size;
#ifdef __cplusplus
	string() : data(0), size(0) {}
	string(const char* _data, int _size) : data((char*)_data), size(_size) {}
	string(const char* _cstr) : data((char*)_cstr), size(_cstr ? (int)strlen(_cstr) : 0) {}
#endif
} string;

typedef enum HT_TypeKind {
	HT_TypeKind_Float,
	HT_TypeKind_Int,
	HT_TypeKind_Bool,
	HT_TypeKind_AssetRef,
	HT_TypeKind_Struct,
	HT_TypeKind_Array,
	HT_TypeKind_ItemTree,
	HT_TypeKind_String,
	HT_TypeKind_Type,
	HT_TypeKind_COUNT,
	HT_TypeKind_INVALID,
} HT_TypeKind;

typedef struct HT_AssetHandle_* HT_AssetHandle;

typedef struct HT_ItemTree {
	void** buckets;
	u16 buckets_count;
	u16 buckets_capacity;
	i32 count;
	i32 last_bucket_end;
	i32 elems_per_bucket;
} HT_ItemTree;

typedef struct HT_ItemHandleDecoded {
	u16 bucket;
	u16 slot;
	u32 generation;
} HT_ItemHandleDecoded;

// Not actually a pointer - encodes a HT_ItemHandleDecoded structure. Typedef'd to a pointer for type safety and easy comparison.
typedef struct HT_ItemHandle_* HT_ItemHandle;

typedef struct HT_Array {
	void* data;
	i32 count;
	i32 capacity;
} HT_Array;

typedef struct HT_Type {
	HT_TypeKind kind;
	HT_TypeKind subkind; // for arrays, this is the element type
	HT_AssetHandle _struct;
} HT_Type;

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

// typedef enum HT_AlignH { HT_AlignH_Left, HT_AlignH_Middle, HT_AlignH_Right } HT_AlignH;

#ifdef __cplusplus
#define HT_EXPORT extern "C" __declspec(dllexport)
#else
#define HT_EXPORT __declspec(dllexport)
#endif

typedef struct HT_GeneratedTypeTable HT_GeneratedTypeTable;

typedef struct HT_AssetViewerTabUpdate {
	HT_AssetHandle data_asset;
	ivec2 rect_min;
	ivec2 rect_max;
} HT_AssetViewerTabUpdate;

typedef struct HT_TabClass HT_TabClass;

typedef struct HT_CustomTabUpdate {
	HT_TabClass* tab_class;
	ivec2 rect_min;
	ivec2 rect_max;
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
	// HT_EXPORT void HT_BuildPluginD3DCommandList(HT_API* ht, ID3D12GraphicsCommandList* command_list)
	
	void (*DebugPrint)(const char* str);

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
	
	const HT_GeneratedTypeTable* type_table;
	
	// Returns NULL if data asset is invalid or of different type than `type_id`
	void* (*GetPluginData)(/*HT_AssetHandle type_id*/);

	// Works on data assets. Returns NULL if asset ref is invalid or not a data asset.
	void* (*AssetGetData)(HT_AssetHandle asset);

	// Works on data assets. Returns NULL if asset ref is invalid or not a data asset.
	HT_AssetHandle (*AssetGetType)(HT_AssetHandle asset);

	// bool (*AssetIsValid)(/*HT_AssetHandle type_id, */HT_AssetHandle asset);
	
	// Returns an empty string if the asset is invalid, otherwise an absolute filepath to the asset.
	// The returned string is valid for this frame only.
	string (*AssetGetFilepath)(HT_AssetHandle asset);
	
	// Whenever an asset data is changed, the modtime integer becomes larger.
	// For folders, the modtime is always the maximum modtime of any asset inside it (applies recursively).
	// Returns 0 if the asset ref is invalid.
	u64 (*AssetGetModtime)(HT_AssetHandle asset);
	
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
	
	bool (*RegisterAssetViewerForType)(HT_AssetHandle struct_type_asset);
	void (*DeregisterAssetViewerForType)(HT_AssetHandle struct_type_asset);
	bool (*PollNextAssetViewerTabUpdate)(HT_AssetViewerTabUpdate* tab_update);
	
	// -- Input --------------------------------------
	
	const HT_InputFrame* input_frame;
	
	// -- UI -----------------------------------------
	
	HT_TabClass* (*CreateTabClass)(string name);
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
	
	// -- D3D12 API -----------------------------------
	
#ifdef HT_INCLUDE_D3D12_API

	ID3D12Device* D3D_device;
	ID3D12CommandQueue* D3D_queue;
	
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

// This header acts as the common interface between Hatch and any plugins that call into Hatch.

#pragma once

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

typedef struct string {
	char* data;
	int size;
#ifdef __cplusplus
	string() : data(0), size(0) {}
	string(const char* _data, int _size) : data((char*)_data), size(_size) {}
	string(const char* _cstr) : data((char*)_cstr), size(_cstr ? (int)strlen(_cstr) : 0) {}
#endif
} string;

typedef struct HT_Color {
	u8 r, g, b, a;
} HT_Color;
#define HT_COLOR HT_LangAgnosticLiteral(HT_Color)

typedef struct HT_DrawVertex {
	vec2 position;
	vec2 uv; // When no texture is used, the uv must be {0, 0}
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

typedef struct HT_Texture HT_Texture;

typedef enum HT_AlignH { HT_AlignH_Left, HT_AlignH_Middle, HT_AlignH_Right } HT_AlignH;

#ifdef __cplusplus
#define HT_EXPORT extern "C" __declspec(dllexport)
#else
#define HT_EXPORT __declspec(dllexport)
#endif

struct HT_API {
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
	
	// -- Memory allocation ---------------------------
	
	// AllocatorProc is a combination of malloc, free and realloc.
	// A new allocation is made (or an existing allocation is resized if ptr != NULL) when new_size > 0.
	// An existing allocation is freed when new_size == 0.
	// All allocations are aligned to 16 bytes.
	void* (*AllocatorProc)(void* ptr, size_t size);
	
	// The returned memory from TempArenaPush gets automatically free'd after the current frame.
	// The returned memory is uninitialized.
	void* (*TempArenaPush)(size_t size, size_t align);
	
	// -- UI functions --------------------------------
	
	// !!! WARNING !!! pointers received from previous calls to AddVertices may be invalid after calling this!
	// TODO: flip the API around to require passing a pointer to vertices and indices.
	HT_DrawVertex* (*AddVertices)(int count, u32* out_first_index);
	
	// Texture may be NULL, in which case the default font atlas will be used.
	// When the uv is {0, 0} and the default font atlas is used, the triangles
	// will read a white pixel from the atlas resulting in untextured geometry.
	// !!! WARNING !!! pointers received from previous calls to AddIndices may be invalid after calling this!
	u32* (*AddIndices)(int count, HT_Texture* texture);
	
	// Maybe we can still have a basic 2D drawing lib built into the API.
	// Like draw text, draw circle, draw rect, etc.
	// void (*DrawText)(string text, vec2 pos, HT_AlignH align, int font_size, HT_Color color);
	
	HT_CachedGlyph (*GetCachedGlyph)(u32 codepoint);
	
	// ------------------------------------------------
	
};

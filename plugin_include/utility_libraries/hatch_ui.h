#include "../hatch_api.h"

#define UI_LIGHTGRAY  HT_COLOR{200, 200, 200, 255}
#define UI_GRAY       HT_COLOR{130, 130, 130, 255}
#define UI_DARKGRAY   HT_COLOR{80, 80, 80, 255}
#define UI_YELLOW     HT_COLOR{253, 249, 0, 255}
#define UI_GOLD       HT_COLOR{255, 203, 0, 255}
#define UI_ORANGE     HT_COLOR{255, 161, 0, 255}
#define UI_PINK       HT_COLOR{255, 109, 194, 255}
#define UI_RED        HT_COLOR{230, 41, 55, 255}
#define UI_MAROON     HT_COLOR{190, 33, 55, 255}
#define UI_GREEN      HT_COLOR{0, 228, 48, 255}
#define UI_LIME       HT_COLOR{0, 158, 47, 255}
#define UI_DARKGREEN  HT_COLOR{0, 117, 44, 255}
#define UI_SKYBLUE    HT_COLOR{102, 191, 255, 255}
#define UI_BLUE       HT_COLOR{0, 121, 241, 255}
#define UI_DARKBLUE   HT_COLOR{0, 82, 172, 255}
#define UI_PURPLE     HT_COLOR{200, 122, 255, 255}
#define UI_VIOLET     HT_COLOR{135, 60, 190, 255}
#define UI_DARKPURPLE HT_COLOR{112, 31, 126, 255}
#define UI_BEIGE      HT_COLOR{211, 176, 131, 255}
#define UI_BROWN      HT_COLOR{127, 106, 79, 255}
#define UI_DARKBROWN  HT_COLOR{76, 63, 47, 255}
#define UI_WHITE      HT_COLOR{255, 255, 255, 255}
#define UI_BLACK      HT_COLOR{0, 0, 0, 255}
#define UI_BLANK      HT_COLOR{0, 0, 0, 0}
#define UI_MAGENTA    HT_COLOR{255, 0, 255, 255}

typedef struct UI_Rect {
	vec2 min, max;
} UI_Rect;
#define UI_RECT HT_LangAgnosticLiteral(UI_Rect)

static void UI_AddQuadIndices(HT_API* ht, u32 a, u32 b, u32 c, u32 d, HT_Texture* texture) {
	u32* indices = ht->AddIndices(6, texture);
	indices[0] = a; indices[1] = b; indices[2] = c;
	indices[3] = a; indices[4] = c; indices[5] = d;
}

static void UI_DrawRect(HT_API* ht, UI_Rect rect, HT_Color color) {
	if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
		u32 first_vert;
		HT_DrawVertex* v = ht->AddVertices(4, &first_vert);
		v[0] = HT_DRAW_VERTEX{{rect.min.x, rect.min.y}, {0, 0}, color};
		v[1] = HT_DRAW_VERTEX{{rect.max.x, rect.min.y}, {0, 0}, color};
		v[2] = HT_DRAW_VERTEX{{rect.max.x, rect.max.y}, {0, 0}, color};
		v[3] = HT_DRAW_VERTEX{{rect.min.x, rect.max.y}, {0, 0}, color};
		UI_AddQuadIndices(ht, first_vert, first_vert + 1, first_vert + 2, first_vert + 3, NULL);
	}
}

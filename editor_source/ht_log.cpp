#include "include/ht_common.h"

/*static void LogPrint(STR_View str) {
	g_next_log_message_index++;

	int max_messages = 128;
	if (g_log_messages[g_current_log_arena].count > max_messages) {
		g_current_log_arena = 1 - g_current_log_arena;

		DS_ArenaReset(&g_log_arenas[g_current_log_arena]);
		DS_ArrInit(&g_log_messages[g_current_log_arena], &g_log_arenas[g_current_log_arena]);
	}

	LogMessage message = {0};
	message.index = g_next_log_message_index;
	message.string = STR_Clone(&g_log_arenas[g_current_log_arena], str);
	DS_ArrPush(&g_log_messages[g_current_log_arena], message);
}*/

static void UILogMessage(UI_Key key, const LogMessage* message) {
	UI_Text temp_text;
	UI_TextInit(UI_FrameArena(), &temp_text, message->string);

	UI_Box* row = UI_KBOX(key);
	UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
	UI_PushBox(row);

	STR_View index_str = STR_IntToStr(UI_FrameArena(), message->index);
	//UI_AddBox(UI_KEY1(key), UI_SizePx(35.f), UI_SizeFlex(1.f), UI_BoxFlag_HasText|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground, index_str, 0);
	UI_Box* line_num = UI_KBOX(key);
	UI_AddLabel(line_num, UI_SizeFit(), UI_SizeFlex(1.f), 0, index_str);
	line_num->draw_args = UI_DrawBoxDefaultArgsInit();
	line_num->draw_args->text_color = UI_COLOR{255, 217, 0, 255};

	UI_Box* edit_text_outer_box = UI_KBOX(key);
	UI_AddValText(edit_text_outer_box, UI_SizeFlex(1.f), UI_SizeFit(), &temp_text);
	edit_text_outer_box->flags &= ~UI_BoxFlag_DrawBorder;

	UI_PopBox(row);
}

EXPORT void UILogTab(EditorState* s, UI_Key key, UI_Rect content_rect) {
	vec2 content_rect_size = UI_RectSize(content_rect);
	UI_Box* root = UI_KBOX(key);
	UI_InitRootBox(root, content_rect_size.x, content_rect_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root);
	UI_PushBox(root);

#if 0
	UI_PushScrollableArea(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), UI_Axis_Y);
	//UI_PushScrollableArea(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFlex(1.f), UI_Axis_X);

	//UI_AddButton(UI_KEY(), UI_SizePx(500.f), UI_StructLiteral(UI_Size){500.f, 0.f, 0.f, 1.f}, STR_V("none"));


	for (int i = 0; i < 16; i++) {
		//UI_AddButton(UI_HashInt(UI_KEY(), i), UI_SizePx(500.f), UI_StructLiteral(UI_Size){0.f, 1.f, 0.f, 1.f}, STR_V("none"));
		UI_AddButton(UI_HashInt(UI_KEY1(key), i), UI_SizeFit(), UI_SizeFit(), STR_V("The quick brown fox jumped over the lazy dog."));
	}
	UI_PopScrollableArea();
#endif

	//static int editing_line = -1;

	UI_Box* scroll_area = UI_KBOX(key);
	UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 1);

	//int message_idx = 0;
	//for (int i = g_log_messages[g_current_log_arena].count; i < g_log_messages[1-g_current_log_arena].count; i++) {
	//	UI_Key elem_key = UI_HashInt(UI_KKEY(key), message_idx);
	//	const LogMessage* message = &g_log_messages[1-g_current_log_arena].data[i];
	//	UILogMessage(elem_key, message);
	//	message_idx++;
	//}
	//
	//for (int i = 0; i < g_log_messages[g_current_log_arena].count; i++) {
	//	UI_Key elem_key = UI_HashInt(UI_KKEY(key), message_idx);
	//	const LogMessage* message = &g_log_messages[g_current_log_arena].data[i];
	//	UILogMessage(elem_key, message);
	//	message_idx++;
	//}

	UI_PopScrollArea(scroll_area);

	UI_PopBox(root);
	UI_BoxComputeRects(root, content_rect.min);
	UI_DrawBox(root);
}

#include "include/ht_common.h"

EXPORT void LogAny(EditorState* s, LogMessageKind kind, const char* fmt, va_list args) {
	STR_Builder b = { TEMP };
	STR_PrintVA(&b, fmt, args);
	
	for (STR_View line, tail = b.str; STR_ParseToAndSkip(&tail, '\n', &line);) {
		LogMessage msg;
		msg.kind = kind;
		msg.string = STR_Clone(&s->log_arena, line);
		DS_ArrPush(&s->log_messages, msg);
	}
}

EXPORT void UpdateAndDrawLogTab(EditorState* s, UI_Key key, UI_Rect area) {
	vec2 area_size = UI_RectSize(area);
	
	UI_Box* root = UI_KBOX(key);
	UI_InitRootBox(root, area_size.x, area_size.y, 0);
	UIRegisterOrderedRoot(&s->dropdown_state, root);
	UI_PushBox(root);

	UI_Box* scroll_area = UI_KBOX(key);
	UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 1);
	
	for (int i = 0; i < s->log_messages.count; i++) {
		LogMessage* msg = &s->log_messages[i];
		
		UI_Text text = {};
		text.text.data = (char*)msg->string.data;
		text.text.count = (int)msg->string.size;
		
		UI_ValTextOptArgs opts = {};
		if (msg->kind != LogMessageKind_Info) {
			UI_Color color = msg->kind == LogMessageKind_Warning ? UI_ORANGE : UI_RED;
			opts.text_color = DS_Dup(UI_TEMP, color);
		}

		UI_Box* msg_box = UI_KBOX(UI_HashInt(key, i));
		UI_AddValText(msg_box, UI_SizeFlex(1.f), UI_SizeFit(), &text, &opts);
		msg_box->flags &= ~UI_BoxFlag_DrawBorder;
		
	}
	
	UI_PopScrollArea(scroll_area);

	UI_PopBox(root);
	UI_BoxComputeRects(root, area.min);
	UI_DrawBox(root);
}

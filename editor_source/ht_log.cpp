#include "include/ht_common.h"
#include "fire/fire_os_timing.h"

EXPORT void LogF(EditorState* s, LogMessageKind kind, const char* fmt, ...) {
	va_list args; va_start(args, fmt);
	LogVArgs(s, kind, fmt, args);
	va_end(args);
}

EXPORT void LogVArgs(EditorState* s, LogMessageKind kind, const char* fmt, va_list args) {
	STR_Builder b = { TEMP };
	STR_PrintVA(&b, fmt, args);
	STR_PrintU(&b, 0);
	printf("STR: `%s`", b.str.data);

	for (STR_View line, tail = b.str; STR_ParseToAndSkip(&tail, '\n', &line);) {
		LogMessage msg;
		msg.kind = kind;
		msg.string = STR_Clone(&s->log_arena, line);
		msg.added_tick = OS_GetCPUTick();
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
	//UI_Vec2 scroll = UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 0);
	UI_Vec2 scroll = UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 1);
	
	float line_height = UI_DEFAULT_TEXT_PADDING.y*2.f + (float)UI_STATE.default_font.size;

	//int start_msg = (int)(scroll.y / line_height);
	//int end_msg = start_msg + (int)(area_size.y / line_height) + 1;
	
	int end_msg = s->log_messages.count - (int)(scroll.y / line_height);
	int start_msg = end_msg - (int)(1.f + area_size.y / line_height);
	start_msg = UI_Max(start_msg, 0);
	end_msg = UI_Min(end_msg, s->log_messages.count);

	UI_AddBox(UI_KBOX(key), 0, (float)start_msg * line_height, 0);

	uint64_t tick = OS_GetCPUTick();

	for (int i = start_msg; i < end_msg; i++) {
		LogMessage* msg = &s->log_messages[i];
		
		UI_Text text = {};
		text.text.data = (char*)msg->string.data;
		text.text.count = (int)msg->string.size;
		
		UI_Color color = UI_WHITE;
		UI_ValTextOptArgs opts = {};
		if (msg->kind != LogMessageKind_Info) {
			color = msg->kind == LogMessageKind_Warning ? UI_ORANGE : UI_RED;
			opts.text_color = DS_Dup(UI_TEMP, color);
		}

		UI_Box* msg_box = UI_KBOX(UI_HashInt(key, i));
		UI_AddValText(msg_box, UI_SizeFlex(1.f), UI_SizeFit(), &text, &opts);

		float highlight = (1.f - (float)OS_GetDuration(s->cpu_frequency, msg->added_tick, tick));
		if (highlight > 0.f) {
			color.a = (uint8_t)(0.2f*highlight * 255.f);
			msg_box->flags |= UI_BoxFlag_DrawTransparentBackground;
			msg_box->draw_opts = DS_New(UI_BoxDrawOptArgs, UI_TEMP);
			msg_box->draw_opts->border_color = DS_Dup(UI_TEMP, color);
			msg_box->draw_opts->transparent_bg_color = DS_Dup(UI_TEMP, color);
		}
		else msg_box->flags &= ~UI_BoxFlag_DrawBorder;
	}
	
	//float end_size_sub = (float)(s->log_messages.count - lazy_msg_count) * line_height;
	//UI_AddBox(UI_KBOX(key), 0, (float)(s->log_messages.count - lazy_msg_count) * line_height, 0);
	//printf("end_size_sub"
	//float end_size =  - end_size_sub;
	UI_AddBox(UI_KBOX(key), 0, (float)(s->log_messages.count - end_msg) * line_height, 0);

	UI_PopScrollArea(scroll_area);

	UI_PopBox(root);
	UI_BoxComputeRects(root, area.min);
	UI_DrawBox(root);
}

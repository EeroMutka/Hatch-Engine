
static inline bool InputIsDown(const HT_InputFrame* in, HT_InputKey key);
static inline bool InputWentDown(const HT_InputFrame* in, HT_InputKey key);
static inline bool InputWentDownOrRepeat(const HT_InputFrame* in, HT_InputKey key);
static inline bool InputWentUp(const HT_InputFrame* in, HT_InputKey key);
static inline bool InputDoubleClicked(const HT_InputFrame* in);
static inline bool InputTripleClicked(const HT_InputFrame* in);

static inline bool InputKeyIsA(HT_InputKey key, HT_InputKey other_key) {
	if (key == other_key) return true;
	if (other_key >= HT_InputKey_Shift && other_key <= HT_InputKey_Super) {
		switch (other_key) {
		case HT_InputKey_Shift:    return key == HT_InputKey_LeftShift   || key == HT_InputKey_RightShift;
		case HT_InputKey_Control:  return key == HT_InputKey_LeftControl || key == HT_InputKey_RightControl;
		case HT_InputKey_Alt:      return key == HT_InputKey_LeftAlt     || key == HT_InputKey_RightAlt;
		case HT_InputKey_Super:    return key == HT_InputKey_LeftSuper   || key == HT_InputKey_RightSuper;
		default: break;
		}
	}
	return false;
}

static inline bool InputIsDown(const HT_InputFrame* in, HT_InputKey key) {
	if (key >= HT_InputKey_Shift && key <= HT_InputKey_Super) {
		switch (key) {
		case HT_InputKey_Shift:    return in->key_is_down[HT_InputKey_LeftShift]   || in->key_is_down[HT_InputKey_RightShift];
		case HT_InputKey_Control:  return in->key_is_down[HT_InputKey_LeftControl] || in->key_is_down[HT_InputKey_RightControl];
		case HT_InputKey_Alt:      return in->key_is_down[HT_InputKey_LeftAlt]     || in->key_is_down[HT_InputKey_RightAlt];
		case HT_InputKey_Super:    return in->key_is_down[HT_InputKey_LeftSuper]   || in->key_is_down[HT_InputKey_RightSuper];
		default: break;
		}
	}
	return in->key_is_down[key];
};

static inline bool InputDoubleClicked(const HT_InputFrame* in) {
	for (int i = 0; i < in->events_count; i++) {
		if (in->events[i].kind == HT_InputEventKind_Press &&
			in->events[i].mouse_click_index == 1) return true;
	}
	return false;
}

static inline bool InputTripleClicked(const HT_InputFrame* in) {
	for (int i = 0; i < in->events_count; i++) {
		if (in->events[i].kind == HT_InputEventKind_Press &&
			in->events[i].mouse_click_index == 2) return true;
	}
	return false;
}

static inline bool InputWentDown(const HT_InputFrame* in, HT_InputKey key) {
	if (InputIsDown(in, key)) {
		for (int i = 0; i < in->events_count; i++) {
			if (in->events[i].kind == HT_InputEventKind_Press && InputKeyIsA(in->events[i].key, key)) {
				return true;
			}
		}
	}
	return false;
}

static inline bool InputWentDownOrRepeat(const HT_InputFrame* in, HT_InputKey key) {
	if (InputIsDown(in, key)) {
		for (int i = 0; i < in->events_count; i++) {
			if ((in->events[i].kind == HT_InputEventKind_Press || in->events[i].kind == HT_InputEventKind_Repeat) && InputKeyIsA(in->events[i].key, key)) {
				return true;
			}
		}
	}
	return false;
}

static inline bool InputWentUp(const HT_InputFrame* in, HT_InputKey key) {
	if (!InputIsDown(in, key)) {
		for (int i = 0; i < in->events_count; i++) {
			if (in->events[i].kind == HT_InputEventKind_Release && InputKeyIsA(in->events[i].key, key)) {
				return true;
			}
		}
	}
	return false;
}

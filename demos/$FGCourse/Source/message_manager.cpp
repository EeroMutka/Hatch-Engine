#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"
#include "ThirdParty/ufbx.h"
#include "ThirdParty/stb_image.h"

#include "message_manager.h"

MessageManager MessageManager::instance{};

void MessageManager::Init() {
	DS_ArrInit(&instance.messages, FG::heap);
}

void MessageManager::Reset() {
	DS_ArrClear(&instance.messages);
}

void MessageManager::SendNewMessageSized(uint64_t type, const Message& message, size_t message_size) {
	void* data = DS_ArenaPush(FG::temp, message_size);
	memcpy(data, &message, message_size);

	StoredMessage stored = {type, data};
	DS_ArrPush(&instance.messages, stored);
}

bool MessageManager::PopNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	for (int i = 0; i < instance.messages.count; i++) {
		if (instance.messages[i].type == type) {
			memcpy(out_message, instance.messages[i].message_data, message_size);
			
			DS_ArrRemove(&instance.messages, i);
			return true;
		}
	}
	return false;
}

bool MessageManager::PeekNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	return false;
}

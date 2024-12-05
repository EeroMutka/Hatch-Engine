#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"
#include "ThirdParty/ufbx.h"
#include "ThirdParty/stb_image.h"

#include "message_manager.h"

MessageManager MessageManager::instance{};

void MessageManager::Init() {
}

void MessageManager::SendNewMessageSized(uint64_t type, const Message& message, size_t message_size) {
	//__debugbreak();
}

bool MessageManager::PopNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	return false;
}

bool MessageManager::PeekNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	return false;
}


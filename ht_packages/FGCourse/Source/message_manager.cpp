#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"
#include "ThirdParty/ufbx.h"
#include "ThirdParty/stb_image.h"

#include "message_manager.h"

#include <Windows.h>
#include <stdio.h>

MessageManager MessageManager::instance{};

void MessageManager::Init() {
	DS_ArrInit(&instance.messages, FG::mem.heap);
	OS_MutexInit(&instance.mutex);
	OS_ConditionVarInit(&instance.wait_cond_var);
}

void MessageManager::BeginFrame() {
	OS_MutexLock(&instance.mutex);
	
	DS_ArrClear(&instance.messages);
	
	OS_MutexUnlock(&instance.mutex);
}

void MessageManager::RegisterNewThread()
{
	instance.threads_registered_count++;
}

void MessageManager::ThreadBarrierAllMessagesSent()
{
	OS_MutexLock(&instance.mutex);
	instance.threads_waiting_at_meetup++;

	if (instance.threads_waiting_at_meetup == instance.threads_registered_count) {
		instance.threads_waiting_at_meetup = 0;
		
		OS_MutexUnlock(&instance.mutex);
		OS_ConditionVarBroadcast(&instance.wait_cond_var);
	}
	else {
		OS_ConditionVarWait(&instance.wait_cond_var, &instance.mutex);
		OS_MutexUnlock(&instance.mutex);
	}
}

void MessageManager::SendNewMessageSized(uint64_t type, const Message& message, size_t message_size) {
	OS_MutexLock(&instance.mutex);
	
	void* data = DS_ArenaPush(FG::mem.temp, message_size);
	memcpy(data, &message, message_size);

	StoredMessage stored = {type, data};
	DS_ArrPush(&instance.messages, stored);
	
	OS_MutexUnlock(&instance.mutex);
}

bool MessageManager::PopNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	OS_MutexLock(&instance.mutex);

	bool result = false;
	for (int i = 0; i < instance.messages.count; i++) {
		if (instance.messages[i].type == type) {
			memcpy(out_message, instance.messages[i].message_data, message_size);
			
			DS_ArrRemove(&instance.messages, i);
			result = true;
			break;
		}
	}
	
	OS_MutexUnlock(&instance.mutex);
	return result;
}

bool MessageManager::PeekNextMessageSized(uint64_t type, Message* out_message, size_t message_size) {
	__debugbreak(); // TODO
	return false;
}

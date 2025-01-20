#include <typeinfo>

class Message {};

// NOTE: All remaining messaged are cleaned up at the end of each frame!
class MessageManager {
public:
	static void Init();
	static void BeginFrame();
	
	static void RegisterNewThread();
	
	// This must be called some time during a frame. All threads meet up at this function at the same time.
	// Alternatively, we could split this into two functions: ThreadHasSentAllMessages & ThreadWaitForAllExternalMessages
	static void ThreadBarrierAllMessagesSent();
	
	template<typename T>
	static inline void SendNewMessage(const T& message) {
		SendNewMessageSized(typeid(message).hash_code(), message, sizeof(message));
	}
	
	template<typename T>
	static bool PeekNextMessage(T* out_message) {
		return PeekNextMessageSized(typeid(*out_message).hash_code(), out_message, sizeof(*out_message));
	}

	template<typename T>
	static bool PopNextMessage(T* out_message) {
		return PopNextMessageSized(typeid(*out_message).hash_code(), out_message, sizeof(*out_message));
	}

private:

	struct StoredMessage {
		uint64_t type;
		void* message_data;
	};

	static bool PeekNextMessageSized(uint64_t type, Message* out_message, size_t message_size);
	static bool PopNextMessageSized(uint64_t type, Message* out_message, size_t message_size);
	static void SendNewMessageSized(uint64_t type, const Message& message, size_t message_size);

	static MessageManager instance;

	OS_Mutex mutex;

	OS_ConditionVar wait_cond_var;

	int threads_waiting_at_meetup;
	int threads_registered_count;
	
	// Optimization idea: instead of having one array, make a map of arrays (one per message type) and increment the starting index when popping a message
	DS_DynArray<StoredMessage> messages;
};

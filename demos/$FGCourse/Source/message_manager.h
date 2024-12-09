#include <typeinfo>

class Message {};

// NOTE: All remaining messaged are cleaned up at the end of each frame!
class MessageManager {
public:
	static void Init();
	static void Reset();
	
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
	
	DS_DynArray<StoredMessage> messages;
};

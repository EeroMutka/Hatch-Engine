
struct PlaySoundMessage : Message {
	float add_pitch;
};

class AudioManager {
public:
	static void Init();

private:
	static AudioManager instance;
	
	OS_Thread audio_thread;
};

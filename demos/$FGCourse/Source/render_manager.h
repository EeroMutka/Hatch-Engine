
struct RenderObjectMessage : Message {
	mat4 local_to_world;
	HT_Asset mesh;
	HT_Asset color_texture;
};

class RenderManager {
public:
	static void Init();
	static void RenderAllQueued();

private:
	static RenderManager instance;
};

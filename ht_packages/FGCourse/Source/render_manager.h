
struct Vertex {
	vec3 position;
	vec2 uv;
	vec3 normal;
};

struct RenderMeshPart {
	u32 index_count;
	u32 base_index_location;
	u32 base_vertex_location;
};

struct RenderMesh {
	void* vbo; // ID3D11Buffer
	void* ibo; // ID3D11Buffer
	DS_DynArray(RenderMeshPart) parts;
};

struct RenderTexture {
	void* texture; // ID3D11Texture2D
	void* texture_srv; // ID3D11ShaderResourceView
};

struct RenderParamsMessage : Message {
	HT_Rect rect;
	mat4 world_to_clip;
	
	// these are passed for gizmo rendering
	Scene__Scene* scene;
	struct SceneEditState* scene_edit_state;
};

struct RenderObjectMessage : Message {
	mat4 local_to_world;
	const RenderMesh* mesh;
	const RenderTexture* color_texture;
};

class RenderManager {
public:
	static void Init();

	static void CreateTexture(RenderTexture* target, int width, int height, void* data);
	
	// Fills in the `vbo` and `ibo` fields of the target mesh
	static void CreateMesh(RenderMesh* target, Vertex* vertices, int num_vertices, u32* indices, int num_indices);

private:
	static RenderManager instance;
};

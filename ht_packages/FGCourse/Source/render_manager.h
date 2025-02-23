
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
	RenderMeshPart part;
	// DS_DynArray<RenderMeshPart> parts; // TODO
	size_t memory_usage;
};

struct RenderTexture {
	void* texture; // ID3D11Texture2D
	void* texture_srv; // ID3D11ShaderResourceView
};

struct AddPointLightMessage : Message {
	vec3 position;
	vec3 emission;
};

struct AddDirectionalLightMessage : Message {
	vec3 rotation;
	vec3 emission;
};

struct AddSpotLightMessage : Message {
	vec3 position;
	vec3 direction;
	vec3 emission;
};

struct RenderParamsMessage : Message {
	HT_Rect rect;
	mat4 world_to_clip;
	vec3 view_position;
	
	// these are passed for gizmo rendering
	Scene__Scene* scene;
	struct SceneEditState* scene_edit_state;
};

struct RenderObjectMessage : Message {
	mat4 local_to_world;
	const RenderMesh* mesh;
	const RenderTexture* color_texture;
	const RenderTexture* specular_texture; // may be null
	float specular_value; // used if specular_texture is null
};

enum class RenderTextureFormat {
	RGBA8,
	R8,
};

class RenderManager {
public:
	static void Init();

	static void CreateTexture(RenderTexture* target, RenderTextureFormat format, int width, int height, void* data);
	
	// Fills in the `vbo` and `ibo` fields of the target mesh
	static void CreateMesh(RenderMesh* target, Vertex* vertices, int num_vertices, u32* indices, int num_indices);

private:
	static RenderManager instance;
};

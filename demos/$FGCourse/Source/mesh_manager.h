
struct Vertex {
	vec3 position;
	vec2 uv;
	vec3 normal;
};

struct MeshPart {
	u32 index_count;
	u32 base_index_location;
	u32 base_vertex_location;
};
	
struct Mesh {
	ID3D11Buffer* vbo;
	ID3D11Buffer* ibo;
	DS_DynArray(MeshPart) parts;
};

struct Texture {
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* texture_srv;
};

class MeshManager {
public:
	static void Init();
	static bool GetMeshFromMeshAsset(HT_Asset mesh_asset, Mesh* out_mesh);
	static bool GetColorTextureFromTextureAsset(HT_Asset texture_asset, Texture* out_texture);

private:
	static MeshManager instance;
	DS_Map(HT_Asset, Mesh) meshes;
	DS_Map(HT_Asset, Texture) textures;
};

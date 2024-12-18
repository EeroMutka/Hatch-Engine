
class MeshManager {
public:
	static void Init();
	
	static RenderMesh* GetMeshFromMeshAsset(HT_Asset mesh_asset);
	static RenderTexture* GetColorTextureFromTextureAsset(HT_Asset texture_asset);

private:
	static MeshManager instance;
	DS_Map(HT_Asset, RenderMesh*) meshes;
	DS_Map(HT_Asset, RenderTexture*) textures;
};

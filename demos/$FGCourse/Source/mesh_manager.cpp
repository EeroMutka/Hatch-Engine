#if 0
#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"
#include "ThirdParty/ufbx.h"
#include "ThirdParty/stb_image.h"

#include "mesh_manager.h"

MeshManager MeshManager::instance{};

void MeshManager::Init() {
	DS_MapInit(&instance.meshes, FG::heap);
	DS_MapInit(&instance.textures, FG::heap);
}

static char* TempCString(HT_StringView str) {
	char* data = DS_ArenaPush(FG::temp, str.size + 1);
	memcpy(data, str.data, str.size);
	data[str.size] = 0;
	return data;
}

static Texture ImportTexture(HT_API* ht, HT_StringView file_path) {
	ID3D11Texture2D* texture;
	ID3D11ShaderResourceView* texture_srv;

	const char* file_path_cstr = TempCString(file_path);

	int size_x, size_y, num_components;
	u8* img_data = stbi_load(file_path_cstr, &size_x, &size_y, &num_components, 4);

	D3D11_SUBRESOURCE_DATA texture_initial_data = {};
	texture_initial_data.pSysMem = img_data;
	texture_initial_data.SysMemPitch = size_x * 4; // 4 bytes per pixel

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = size_x;
	desc.Height = size_y;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.SampleDesc.Count = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bool ok = ht->D3D11_device->CreateTexture2D(&desc, &texture_initial_data, &texture) == S_OK;
	HT_ASSERT(ok);

	ok = ht->D3D11_device->CreateShaderResourceView(texture, NULL, &texture_srv) == S_OK;
	HT_ASSERT(ok);

	stbi_image_free(img_data);

	return { texture, texture_srv };
}

// bind_flags: D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_INDEX_BUFFER
static ID3D11Buffer* CreateGPUBuffer(HT_API* ht, int size, u32 bind_flags, void* data) {
	HT_ASSERT(size > 0);

	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth = size;
	desc.Usage = D3D11_USAGE_DYNAMIC; // lets do dynamic for simplicity    D3D11_USAGE_DEFAULT;
	desc.BindFlags = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	// TODO: We can provide initial data by just passing it as the second arg to CreateBuffer!!!!
	ID3D11Buffer* buffer;
	bool ok = ht->D3D11_device->CreateBuffer(&desc, NULL, &buffer) == S_OK;
	HT_ASSERT(ok);

	D3D11_MAPPED_SUBRESOURCE buffer_mapped;
	ok = ht->D3D11_device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &buffer_mapped) == S_OK;
	HT_ASSERT(ok && buffer_mapped.pData);

	memcpy(buffer_mapped.pData, data, size);

	ht->D3D11_device_context->Unmap(buffer, 0);
	return buffer;
}

static Mesh ImportMesh(HT_API* ht, DS_Allocator* allocator, HT_StringView file_path) {
	Mesh result = {};
	DS_ArrInit(&result.parts, allocator);

	ufbx_error error; // Optional, pass NULL if you don't care about errors

	ufbx_load_opts opts = {0}; // Optional, pass NULL for defaults
	opts.generate_missing_normals = true;

	const char* file_path_cstr = TempCString(file_path);
	ufbx_scene* fbx_scene = ufbx_load_file(file_path_cstr, &opts, &error);
	HT_ASSERT(fbx_scene);

	struct TempMeshPart {
		Vertex* vertices;
		u32 base_vertex;
		u32 num_vertices;
		u32* indices;
		u32 base_index;
		u32 num_indices;
	};

	DS_DynArray(TempMeshPart) temp_parts = {FG::temp};
	u32 total_num_vertices = 0;
	u32 total_num_indices = 0;

	for (size_t node_i = 0; node_i < fbx_scene->nodes.count; node_i++) {
		ufbx_node* node = fbx_scene->nodes.data[node_i];
		if (node->is_root) continue;

		if (node->mesh) {
			// see docs at https://ufbx.github.io/elements/meshes/
			ufbx_mesh* fbx_mesh = node->mesh;

			u32 num_tri_indices = (u32)fbx_mesh->max_face_triangles * 3;
			u32* tri_indices = (u32*)DS_ArenaPush(FG::temp, num_tri_indices * sizeof(u32));

			for (u32 part_i = 0; part_i < fbx_mesh->material_parts.count; part_i++) {
				ufbx_mesh_part* part = &fbx_mesh->material_parts[part_i];
				//ufbx_material* material = fbx_mesh->materials[part_i];

				u32 num_triangles = (u32)part->num_triangles;
				Vertex* vertices = (Vertex*)DS_ArenaPush(FG::temp, num_triangles * 3 * sizeof(Vertex));
				u32 num_vertices = 0;

				for (u32 face_i = 0; face_i < part->num_faces; face_i++) {
					ufbx_face face = fbx_mesh->faces.data[part->face_indices.data[face_i]];

					u32 num_triangulated_tris = ufbx_triangulate_face(tri_indices, num_tri_indices, fbx_mesh, face);

					for (u32 i = 0; i < num_triangulated_tris*3; i++) {
						u32 index = tri_indices[i];

						Vertex* v = &vertices[num_vertices++];
						ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
						ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
						ufbx_vec2 uv = ufbx_get_vertex_vec2(&fbx_mesh->vertex_uv, index);
						v->position = {(float)position.x, (float)position.y, (float)position.z};
						v->uv = {(float)uv.x, (float)uv.y};
						v->normal = {(float)normal.x, (float)normal.y, (float)normal.z};
					}
				}

				assert(num_vertices == num_triangles * 3);

				// Generate index buffer
				ufbx_vertex_stream streams[1] = {
					{ vertices, num_vertices, sizeof(Vertex) },
				};
				u32 num_indices = num_triangles * 3;
				u32* indices = (u32*)DS_ArenaPush(FG::temp, num_indices * sizeof(u32));

				// This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
				// indices are written in `indices[]` and the number of unique vertices is returned.
				num_vertices = (u32)ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

				TempMeshPart temp_part = {
					vertices, total_num_vertices, num_vertices,
					indices, total_num_indices, num_indices,
				};
				DS_ArrPush(&temp_parts, temp_part);

				MeshPart loaded_part = {};
				loaded_part.base_index_location = total_num_indices;
				loaded_part.base_vertex_location = total_num_vertices;
				loaded_part.index_count = num_indices;
				DS_ArrPush(&result.parts, loaded_part);

				total_num_indices += num_indices;
				total_num_vertices += num_vertices;
			}
		}
	}

	ufbx_free_scene(fbx_scene);

	// Allocating a temporary "combined" array is unnecessary and required by the `initial_data` API for CreateGPUBuffer.
	// If we instead mapped the buffer pointer, we could copy directly to it. But this is good enough for now.
	Vertex* combined_vertices = (Vertex*)ht->TempArenaPush(total_num_vertices * sizeof(Vertex), 4);
	u32* combined_indices = (u32*)ht->TempArenaPush(total_num_indices * sizeof(u32), 4);

	for (int i = 0; i < temp_parts.count; i++) {
		TempMeshPart temp_part = temp_parts[i];
		memcpy((Vertex*)combined_vertices + temp_part.base_vertex, temp_part.vertices, temp_part.num_vertices * sizeof(Vertex));
		memcpy((u32*)combined_indices + temp_part.base_index, temp_part.indices, temp_part.num_indices * sizeof(u32));
	}

	result.vbo = CreateGPUBuffer(ht, total_num_vertices * sizeof(Vertex), D3D11_BIND_VERTEX_BUFFER, combined_vertices);
	result.ibo = CreateGPUBuffer(ht, total_num_indices * sizeof(u32), D3D11_BIND_INDEX_BUFFER, combined_indices);
	return result;
}

bool MeshManager::GetMeshFromMeshAsset(HT_Asset mesh_asset, Mesh* out_mesh) {
	Mesh* cached = NULL;
	bool added_new = DS_MapGetOrAddPtr(&instance.meshes, mesh_asset, &cached);
	if (added_new) {
		Scene__Mesh* mesh_data = HT_GetAssetData(Scene__Mesh, FG::ht, mesh_asset);
		HT_ASSERT(mesh_data);
		HT_StringView mesh_source_file = FG::ht->AssetGetFilepath(mesh_data->mesh_source);
		*cached = ImportMesh(FG::ht, FG::heap, mesh_source_file);
	}

	*out_mesh = *cached;
	return true;
}

bool MeshManager::GetColorTextureFromTextureAsset(HT_Asset texture_asset, Texture* out_texture) {
	Texture* cached = NULL;
	bool added_new = DS_MapGetOrAddPtr(&instance.textures, texture_asset, &cached);
	if (added_new) {
		Scene__Texture* color_texture_data = HT_GetAssetData(Scene__Texture, FG::ht, texture_asset);
		HT_ASSERT(color_texture_data);
		HT_StringView color_texture_source_file = FG::ht->AssetGetFilepath(color_texture_data->texture_source);
		*cached = ImportTexture(FG::ht, color_texture_source_file);
	}
	
	*out_texture = *cached;
	return true;
}

#endif
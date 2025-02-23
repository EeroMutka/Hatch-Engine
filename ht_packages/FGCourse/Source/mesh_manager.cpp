#define _CRT_SECURE_NO_WARNINGS
#define HT_NO_STATIC_PLUGIN_EXPORTS

#include "common.h"
#include "ThirdParty/ufbx.h"
#include "ThirdParty/stb_image.h"
#include "ht_utils/math/math_core.h"

#include "message_manager.h"
#include "render_manager.h"
#include "mesh_manager.h"

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <Windows.h> // for GlobalMemoryStatus

MeshManager MeshManager::instance{};

void MeshManager::Init() {
	DS_MapInit(&instance.meshes, FG::mem.heap);
	DS_MapInit(&instance.textures, FG::mem.heap);
}

static char* TempCString(HT_StringView str) {
	char* data = DS_ArenaPush(FG::mem.temp, str.size + 1);
	memcpy(data, str.data, str.size);
	data[str.size] = 0;
	return data;
}

static void SerializeMesh(RenderMesh* mesh, FILE* file)
{
	//mesh->part.
}

struct SerializedMeshHeader
{
	uint32_t vertices_count;
	uint32_t indices_count;
};

static RenderMesh* DeserializeMesh(FILE* file)
{
	RenderMesh* result = new RenderMesh();
	SerializedMeshHeader header = {};
	
	HT_ASSERT(file);
	fread(&header, 1, sizeof(header), file);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	vertices.resize(header.vertices_count);
	indices.resize(header.indices_count);

	fread(vertices.data(), 1, sizeof(Vertex) * vertices.size(), file);
	fread(indices.data(), 1, sizeof(uint32_t) * indices.size(), file);
	
	RenderManager::CreateMesh(result, vertices.data(), (uint32_t)vertices.size(), indices.data(), (uint32_t)indices.size());
	
	result->part.index_count = (uint32_t)indices.size();
	result->memory_usage = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t);
	return result;
}

static RenderMesh* ImportOBJMeshAndSerializeToBinary(HT_API* ht, HT_StringView file_path, const char* binary_file_path) {
	RenderMesh* result = new RenderMesh();

	std::string file_path_str(file_path.data, file_path.size);

	std::ifstream file(file_path_str);
	std::string line;

	HT_ASSERT(file.is_open());

	std::vector<vec3> positions;
	std::vector<vec2> uvs;
	std::vector<vec3> normals;
	std::vector<uint32_t> position_indices;
	std::vector<uint32_t> uv_indices;
	std::vector<uint32_t> normal_indices;

	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string prefix;
		iss >> prefix;
		if (prefix == "v") {
			vec3 v;
			iss >> v.x >> v.y >> v.z;
			positions.push_back(v);
		}

		if (prefix == "vt") {
			vec2 vt;
			iss >> vt.x >> vt.y;
			uvs.push_back(vt);
		}

		if (prefix == "vn") {
			vec3 vn;
			iss >> vn.x >> vn.y >> vn.z;
			normals.push_back(vn);
		}

		if (prefix == "f") {
			for (int i = 0; i < 3; i++) {
				std::string vert_word_str;
				iss >> vert_word_str;
				if (vert_word_str.size() == 0) break;

				std::istringstream vert_word(vert_word_str);
				int position_index;
				int uv_index;
				int normal_index;

				vert_word >> position_index;
				vert_word.ignore(1);
				vert_word >> uv_index;
				vert_word.ignore(1);
				vert_word >> normal_index;
				vert_word.ignore(1);

				position_indices.push_back(position_index - 1);
				uv_indices.push_back(uv_index - 1);
				normal_indices.push_back(normal_index - 1);
			}
		}
	}

	file.close();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	struct VertexIndices {
		uint16_t a, b, c, d;
	};
	std::unordered_map<int64_t, int> map;

	for (int i = 0; i < position_indices.size(); i++) {
		int pos_i = position_indices[i];
		int uv_i = uv_indices[i];
		int normal_i = normal_indices[i];

		VertexIndices ii = {
			(uint16_t)pos_i,
			(uint16_t)uv_i,
			(uint16_t)normal_i,
			0,
		};
		uint64_t ii_as_u64 = *(uint64_t*)&ii;

		vec3 pos = positions[pos_i];
		vec2 uv = uvs[uv_i];
		vec3 normal = normals[normal_i];
		Vertex v = { pos, uv, normal };

		auto found = map.find(ii_as_u64);
		if (found == map.end()) {
			map[ii_as_u64] = (uint32_t)vertices.size();
			indices.push_back((uint32_t)vertices.size());
			vertices.push_back(v);
		}
		else {
			indices.push_back(found->second);
		}
	}

	RenderManager::CreateMesh(result, vertices.data(), (uint32_t)vertices.size(), indices.data(), (uint32_t)indices.size());

	// Serialize to binary
	{
		SerializedMeshHeader header = {};
		header.indices_count = (uint32_t)indices.size();
		header.vertices_count = (uint32_t)vertices.size();
		
		FILE* f = fopen(binary_file_path, "wb");
		HT_ASSERT(f);
		fwrite(&header, 1, sizeof(header), f);
		fwrite(vertices.data(), 1, sizeof(Vertex) * vertices.size(), f);
		fwrite(indices.data(), 1, sizeof(uint32_t) * indices.size(), f);
		fclose(f);
	}

	result->part.index_count = (uint32_t)indices.size();
	result->memory_usage = vertices.size() * sizeof(Vertex) + indices.size() * sizeof(uint32_t);
	return result;
}

#if 0
static RenderMesh* ImportMesh(HT_API* ht, DS_Allocator* allocator, HT_StringView file_path) {
	RenderMesh* result = new RenderMesh();

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

	DS_DynArray(TempMeshPart) temp_parts = {FG::mem.temp};
	u32 total_num_vertices = 0;
	u32 total_num_indices = 0;

	for (size_t node_i = 0; node_i < fbx_scene->nodes.count; node_i++) {
		ufbx_node* node = fbx_scene->nodes.data[node_i];
		if (node->is_root) continue;

		if (node->mesh) {
			// see docs at https://ufbx.github.io/elements/meshes/
			ufbx_mesh* fbx_mesh = node->mesh;

			u32 num_tri_indices = (u32)fbx_mesh->max_face_triangles * 3;
			u32* tri_indices = (u32*)DS_ArenaPush(FG::mem.temp, num_tri_indices * sizeof(u32));

			for (u32 part_i = 0; part_i < fbx_mesh->material_parts.count; part_i++) {
				ufbx_mesh_part* part = &fbx_mesh->material_parts[part_i];
				//ufbx_material* material = fbx_mesh->materials[part_i];

				u32 num_triangles = (u32)part->num_triangles;
				Vertex* vertices = (Vertex*)DS_ArenaPush(FG::mem.temp, num_triangles * 3 * sizeof(Vertex));
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
				u32* indices = (u32*)DS_ArenaPush(FG::mem.temp, num_indices * sizeof(u32));

				// This call will deduplicate vertices, modifying the arrays passed in `streams[]`,
				// indices are written in `indices[]` and the number of unique vertices is returned.
				num_vertices = (u32)ufbx_generate_indices(streams, 1, indices, num_indices, NULL, NULL);

				TempMeshPart temp_part = {
					vertices, total_num_vertices, num_vertices,
					indices, total_num_indices, num_indices,
				};
				DS_ArrPush(&temp_parts, temp_part);

				RenderMeshPart loaded_part = {};
				loaded_part.base_index_location = total_num_indices;
				loaded_part.base_vertex_location = total_num_vertices;
				loaded_part.index_count = num_indices;
				
				HT_ASSERT(temp_parts.count == 1); // TODO: multiple parts
				result->part = loaded_part;
				//DS_ArrPush(&result->parts, loaded_part);

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

	RenderManager::CreateMesh(result, combined_vertices, total_num_vertices, combined_indices, total_num_indices);

	HT_ASSERT(temp_parts.count == 1);

	return result;
}
#endif

RenderMesh* MeshManager::GetMeshFromMeshAsset(HT_Asset mesh_asset) {
	RenderMesh** cached = NULL;
	bool added_new = DS_MapGetOrAddPtr(&instance.meshes, mesh_asset, &cached);
	if (added_new) {
		Scene__Mesh* mesh_data = HT_GetAssetData(Scene__Mesh, FG::ht, mesh_asset);
		HT_ASSERT(mesh_data);
		
		HT_StringView mesh_source_file = FG::ht->AssetGetFilepath(mesh_data->mesh_source);
		
		std::string mesh_source_file_str(mesh_source_file.data, mesh_source_file.size);
		std::string mesh_bin_filepath = mesh_source_file_str.substr(0, mesh_source_file_str.find('.')) + ".mesh_binary";
		
		FILE* mesh_bin_file = fopen(mesh_bin_filepath.c_str(), "rb");
		if (mesh_bin_file)
		{
			*cached = DeserializeMesh(mesh_bin_file);
			fclose(mesh_bin_file);
		}
		else
		{
			*cached = ImportOBJMeshAndSerializeToBinary(FG::ht, mesh_source_file, mesh_bin_filepath.c_str());
			
			//*cached = ImportMesh(FG::ht, FG::mem.heap, mesh_source_file);
		}

		MEMORYSTATUS mem_status;
		GlobalMemoryStatus(&mem_status);
		HT_LogInfo("Loading mesh \"%s\" - using memory %f MB (avail: %f MB, total: %f MB)",
			mesh_source_file_str.c_str(),
			(float)(*cached)->memory_usage / (1024.f*1024.f),
			(float)mem_status.dwAvailPhys / (1024.f*1024.f),
			(float)mem_status.dwTotalPhys / (1024.f*1024.f));
	}
	return *cached;
}

RenderTexture* MeshManager::GetTextureFromTextureAsset(HT_Asset texture_asset, RenderTextureFormat format) {
	RenderTexture** cached = NULL;
	bool added_new = DS_MapGetOrAddPtr(&instance.textures, texture_asset, &cached);
	if (added_new) {
		Scene__Texture* color_texture_data = HT_GetAssetData(Scene__Texture, FG::ht, texture_asset);
		HT_ASSERT(color_texture_data);
		HT_StringView file_path = FG::ht->AssetGetFilepath(color_texture_data->texture_source);
		
		const char* file_path_cstr = TempCString(file_path);
		int size_x, size_y, num_components;

		int want_channels = format == RenderTextureFormat::RGBA8 ? 4 : 1;
		u8* img_data = stbi_load(file_path_cstr, &size_x, &size_y, &num_components, want_channels);

		RenderTexture* texture = new RenderTexture();
		RenderManager::CreateTexture(texture, format, size_x, size_y, img_data);

		stbi_image_free(img_data);

		*cached = texture;
	}
	return *cached;
}

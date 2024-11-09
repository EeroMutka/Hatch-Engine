#include "include/ht_internal.h"

#define MD_FUNCTION extern "C"
#include "third_party/md.h"

static STR_View GetAssetExt(AssetKind kind) {
	switch (kind) {
	case AssetKind_Plugin:     return ".plugin.ht";
	case AssetKind_StructType: return ".struct.ht";
	case AssetKind_StructData: return ".data.ht";
	default: return "";
	}
}

EXPORT STR_View AssetGetFilename(DS_Arena* arena, Asset* asset) {
	STR_View result = STR_Form(arena, "%v%v", UI_TextToStr(asset->name), GetAssetExt(asset->kind));
	return result;
}

EXPORT STR_View AssetGetFilepath(DS_Arena* arena, Asset* asset) {
	if (asset->kind == AssetKind_Package) {
		return STR_Clone(arena, asset->package.filesys_path);
	}
	
	STR_View result = AssetGetFilename(TEMP, asset);
	for (Asset* p = asset->parent; p->kind != AssetKind_Root; p = p->parent) {
		result = p->kind == AssetKind_Package ?
			STR_Form(arena, "%v/%v", p->package.filesys_path, result) :
			STR_Form(TEMP, "%v/%v", UI_TextToStr(p->name), result);
	}
	return result;
}

EXPORT STR_View AssetGetFilepathUsingParentDirectory(DS_Arena* arena, STR_View directory, Asset* asset) {
	STR_View result = STR_Form(arena, "%v/%v%v", directory, UI_TextToStr(asset->name), GetAssetExt(asset->kind));
	return result;
}

static inline STR_View StrFromMD(MD_String8 str) { return {(char*)str.str, str.size}; }
static inline MD_String8 StrToMD(STR_View str) { return {(MD_u8*)str.data, (MD_u64)str.size}; }

struct ReloadAssetsContext {
	AssetTree* tree;
	Asset* package;
	MD_Arena* md_arena;
	DS_DynArray(Asset*) queue_recompile_plugins;
};

// While we go, we also generate and update filesys_paths.
static void SaveAsset(Asset* asset, STR_View filesys_path) {
	assert(!STR_ContainsU(filesys_path, '\\'));

	if (asset->kind == AssetKind_Folder || asset->kind == AssetKind_Package) {
		bool ok = OS_MakeDirectory(MEM_TEMP(), filesys_path);
		assert(ok);

		OS_FileInfoArray files;
		ok = OS_GetAllFilesInDirectory(MEM_SCOPE(TEMP), filesys_path, &files);

		DS_Map(uint64_t, Asset*) asset_from_name;
		DS_MapInit(&asset_from_name, TEMP);

		for (Asset* child = asset->first_child; child; child = child->next) {
			//STR_View filesys_name = STR_AfterLast(child->filesys_path, '/');
			STR_View child_name = UI_TextToStr(child->name);
			uint64_t hash = DS_MurmurHash64A(child_name.data, child_name.size, 0);
			DS_MapInsert(&asset_from_name, hash, child);
		}

		// Delete files which exist in the filesystem, but aren't part of the assets
		for (int i = 0; i < files.count; i++) {
			OS_FileInfo* info = &files.data[i];
			if (info->is_directory && STR_MatchCaseInsensitive(info->name, ".plugin_binaries")) continue;

			STR_View stem = STR_BeforeFirst(info->name, '.');
			uint64_t hash = DS_MurmurHash64A(stem.data, stem.size, 0);

			if (DS_MapFindPtr(&asset_from_name, hash) == NULL) {
				if (info->is_directory) {
					OS_DeleteDirectory(MEM_TEMP(), info->name);
				} else {
					OS_DeleteFile(MEM_TEMP(), info->name);
				}
			}
		}

		for (Asset* child = asset->first_child; child; child = child->next) {
			STR_View child_filesys_path = AssetGetFilepathUsingParentDirectory(TEMP, filesys_path, child);
			SaveAsset(child, child_filesys_path);
		}
	}
	else {
		bool write = true;

		if (asset->kind == AssetKind_File) { // only write file assets when they don't exist already
			uint64_t modtime;
			bool file_exists = OS_FileGetModtime(MEM_TEMP(), filesys_path, &modtime);
			write = file_exists == false;
		}

		FILE* file = NULL;
		if (write) file = fopen(STR_ToC(TEMP, filesys_path), "wb");
		if (file) {
			// Serialize file
			if (asset->kind == AssetKind_Plugin || asset->kind == AssetKind_StructType) {
				fprintf(file, "struct: {\n");
				for (int i = 0; i < asset->struct_type.members.count; i++) {
					TODO();
					//StructMember* member = &asset->struct_type.members.data[i];
					//STR_View type_str = PluginVarTypeToString(member->type);
					//fprintf(file, "\t%.*s: %.*s,\n", StrArg(member->name.str), StrArg(type_str));
					fprintf(file, "\tTODO,\n");
				}
				fprintf(file, "}\n\n");
			}

			if (asset->kind == AssetKind_Plugin || asset->kind == AssetKind_StructData) {
				//fprintf(file, "data_asset: {\n");
					TODO();
				//fprintf(file, "\tTODO,\n");
				//DataBuffer *data_buffer = DS_ArrGetPtr(g_data_buffers, asset->data_buffer_index);
				//char *data = data_buffer->base;
				//uint32_t offset = sizeof(DataBufferHeader) + 8;
				//
				//for (int i = 0; i < asset->struct_members.count; i++) {
				//	StructMember* member = &asset->struct_members.data[i];
				//	switch (member->type) {
				//	case Type_Float: {
				//		fprintf(file, "\t%.*s: %f,\n", StrArg(member->name.str), *(float*)(data + offset));
				//	}break;
				//	case Type_Int: {
				//		fprintf(file, "\t%.*s: %d,\n", StrArg(member->name.str), *(int*)(data + offset));
				//	}break;
				//	case Type_Bool: {
				//		fprintf(file, "\t%.*s: %s,\n", StrArg(member->name.str), *(int*)(data + offset) ? "true" : "false");
				//	}break;
				//	}
				//	offset += 4;
				//}
				fprintf(file, "}\n\n");
			}

			if (asset->kind == AssetKind_Plugin) {
				fprintf(file, "source_files: {\n");
				fprintf(file, "\tTODO,\n");
				TODO();
				//for (int i = 0; i < asset->plugin_source_files.count; i++) {
				//	AssetHandle source_file_handle = asset->plugin_source_files.data[i];
				//	STR_View source_file_path = AssetHandleIsValid(source_file_handle) ? ComputeAssetPath(TEMP, source_file_handle.ptr) : STR_("");
				//
				//	fprintf(file, "\t\"%.*s\",\n", StrArg(source_file_path));
				//}

				fprintf(file, "}\n");
			}

			fclose(file);
		}
	}
}

EXPORT void SavePackageToDisk(Asset* package) {
	assert(package->kind == AssetKind_Package);

	if (package->package.filesys_path.size == 0) {
		STR_View filesys_path;
		bool ok = OS_FolderPicker(MEM_SCOPE(TEMP), &filesys_path);
		if (!ok) {
			printf("Invalid path selected!\n"); // TODO: use log window
			return;
		}
		package->package.filesys_path = STR_Clone(DS_HEAP, filesys_path);
	}
	
	OS_SetWorkingDir(MEM_TEMP(), package->package.filesys_path);

	SaveAsset(package, package->package.filesys_path);
}

static void ReloadAssetsPass1(AssetTree* tree, Asset* package, Asset* parent, STR_View parent_full_path) {
	OS_FileInfoArray files = {0};
	OS_GetAllFilesInDirectory(MEM_SCOPE(TEMP), parent_full_path, &files);
	
	DS_Map(uint64_t, int) file_idx_from_name;
	DS_MapInit(&file_idx_from_name, TEMP);
	
	for (int i = 0; i < files.count; i++) {
		OS_FileInfo* info = &files.data[i];
		uint64_t hash = DS_MurmurHash64A(info->name.data, info->name.size, 0);
		DS_MapInsert(&file_idx_from_name, hash, i);
	}
	
	DS_DynArray(Asset*) asset_from_file_idx;
	DS_ArrInit(&asset_from_file_idx, TEMP);
	
	Asset* null_asset = NULL;
	DS_ArrResize(&asset_from_file_idx, null_asset, files.count);

	// First delete assets which do not exist in the filesystem
	for (Asset* asset = parent->first_child; asset;) {
		Asset* next = asset->next;
		
		STR_View name = AssetGetFilename(TEMP, asset);
		uint64_t hash = DS_MurmurHash64A(name.data, name.size, 0);
		
		int file_idx;
		if (DS_MapFind(&file_idx_from_name, hash, &file_idx)) {
			asset_from_file_idx[file_idx] = asset;
		}
		else {
			DeleteAssetIncludingChildren(tree, asset);
		}
		asset = next;
	}

	// Then add assets that don't exist in the asset system
	for (int i = 0; i < files.count; i++) {
		OS_FileInfo info = files.data[i];
		if (info.is_directory && STR_MatchCaseInsensitive(info.name, ".plugin_binaries")) continue;
		
		STR_View file_name = {info.name.data, info.name.size};

		Asset* asset = DS_ArrGet(asset_from_file_idx, i);
		if (asset == NULL) {
			STR_View name = STR_AfterLast(file_name, '/');
			STR_View stem = STR_BeforeFirst(name, '.');
			STR_View ext = STR_AfterFirst(name, '.');
			if (STR_MatchCaseInsensitive(ext, "inc.ht")) continue;

			AssetKind asset_kind = AssetKind_Folder;
			if (!info.is_directory) {
				/**/ if (STR_MatchCaseInsensitive(ext, "plugin.ht")) asset_kind = AssetKind_Plugin;
				else if (STR_MatchCaseInsensitive(ext, "struct.ht")) asset_kind = AssetKind_StructType;
				else if (STR_MatchCaseInsensitive(ext, "data.ht"))   asset_kind = AssetKind_StructData;
				else asset_kind = AssetKind_File;
			}

			asset = MakeNewAsset(tree, asset_kind);

			//asset->has_unsaved_changes = false;
			UI_TextSet(&asset->name, asset_kind == AssetKind_File ? name : stem);
			
			MoveAssetToInside(tree, asset, parent);
		}

		assert(info.last_write_time >= asset->modtime); // modtime should never decrease on windows
		asset->reload_assets_pass2_needs_hotreload = info.last_write_time != asset->modtime;
		asset->modtime = info.last_write_time;

		if (info.last_write_time != asset->modtime) { // propagate modtime up through the parent folders
			for (Asset* p = parent; p; p = p->parent) {
				if (info.last_write_time > p->modtime) p->modtime = info.last_write_time;
			}
		}

		STR_View full_path = STR_Form(TEMP, "%v/%v", parent_full_path, file_name);
		asset->reload_assets_filesys_path = full_path;

		if (info.is_directory) {
			ReloadAssetsPass1(tree, package, asset, full_path);
		}
	}
}

static bool ParseMetadeskInt(MD_Node *node, int *out_value) {
	int sign = 1;
	if (node->flags & MD_NodeFlag_Symbol && MD_S8Match(node->string, MD_S8Lit("-"), 0)) {
		node = node->next;
		sign = -1;
	}
	if (MD_NodeIsNil(node)) return false;
	if (!(node->flags & MD_NodeFlag_Numeric)) return false;
	if (!MD_StringIsCStyleInt(node->string)) return false;
	
	*out_value = sign * (int)MD_CStyleIntFromString(node->string);
	return true;
}

static bool ParseMetadeskFloat(MD_Node* node, float *out_value) {
	float sign = 1.f;
	if (node->flags & MD_NodeFlag_Symbol && MD_S8Match(node->string, MD_S8Lit("-"), 0)) {
		node = node->next;
		sign = -1.f;
	}
	if (MD_NodeIsNil(node)) return false;
	if (!(node->flags & MD_NodeFlag_Numeric)) return false;
	
	if (MD_StringIsCStyleInt(node->string)) {
		*out_value = sign * (float)MD_CStyleIntFromString(node->string);
		return true;
	}

	double value;
	if (!STR_ParseFloat(StrFromMD(node->string), &value)) return false;

	*out_value = sign * (float)value;
	return true;
}

static void ReloadAssetsPass2(ReloadAssetsContext* ctx, Asset* parent) {
	for (Asset* asset = parent->first_child; asset; asset = asset->next) {
		if (!asset->reload_assets_pass2_needs_hotreload) continue;

		MD_ParseResult parse;
		if (asset->kind == AssetKind_StructType)
		{
			MD_String8 md_filepath = {(MD_u8*)asset->reload_assets_filesys_path.data, (MD_u64)asset->reload_assets_filesys_path.size};

			MD_ArenaClear(ctx->md_arena);
			parse = MD_ParseWholeFile(ctx->md_arena, md_filepath);
			assert(!MD_NodeIsNil(parse.node));
			assert(parse.errors.node_count == 0);
		}

		// Queue up the plugin for recompilation if any source file has been modified
		//if (asset->kind == AssetKind_Plugin && asset->plugin_active_data) {
		//	DS_ForMapEach(uint64_t, PluginIncludeStamp, &asset->plugin_active_data->includes, IT) {
		//		OS_FileTime last_write_time;
		//		assert(OS_FileModtime(IT.value->file_path, &last_write_time));
		//		if (OS_FileCmpModtime(last_write_time, IT.value->last_write_time) == 0) continue;
		//		
		//		DS_ArrPush(&ctx->queue_recompile_plugins, asset);
		//		break;
		//	}
		//}

		
		if (asset->kind == AssetKind_StructType) {
			MD_Node* struct_node = MD_ChildFromString(parse.node, MD_S8Lit("struct"), 0);
			for (int i = 0; i < asset->struct_type.members.count; i++) {
				StructMemberDeinit(&asset->struct_type.members[i]);
			}
			DS_ArrClear(&asset->struct_type.members);
			
			//DS_ArrClear(&asset->struct_members);
			assert(!MD_NodeIsNil(struct_node));
			for (MD_EachNode(it, struct_node->first_child)) {
				StructMember member = {0};
				StructMemberInit(&member);
				
				STR_View name = StrFromMD(it->string);
				UI_TextSet(&member.name.text, name);
				
				assert(!MD_NodeIsNil(it->first_child));
				STR_View type_name = StrFromMD(it->first_child->string);
				member.type.kind = StringToTypeKind(type_name);
				assert(member.type.kind != HT_TypeKind_INVALID);
				
				if (MD_NodeHasTag(it->first_child, MD_S8Lit("Array"), 0)) {
					member.type.subkind = member.type.kind;
					member.type.kind = HT_TypeKind_Array;
				}
				
				DS_ArrPush(&asset->struct_type.members, member);
			}
			
			ComputeStructLayout(ctx->tree, asset);
		}
		
		ReloadAssetsPass2(ctx, asset);
	}
}

// `dst` is expected to be zero-initialized.
static void ReadMetadeskValue(AssetTree* tree, Asset* package, void* dst, HT_Type* type, MD_Node* member_node, MD_Node* value_node) {
	switch (type->kind) {
	case HT_TypeKind_Array: {
		HT_Array* val = (HT_Array*)dst;
		
		assert(member_node != NULL);
		assert(member_node->flags & MD_NodeFlag_HasBraceLeft);
		assert(member_node->flags & MD_NodeFlag_HasBraceLeft);

		HT_Type elem_type = *type;
		elem_type.kind = type->subkind;
		
		i32 elem_size, elem_align;
		GetTypeSizeAndAlignment(tree, &elem_type, &elem_size, &elem_align);

		int i = 0;
		for (MD_Node* child = member_node->first_child; !MD_NodeIsNil(child); child = child->next) {
			ArrayPush(val, elem_size);
			char* elem_data = (char*)val->data + elem_size*i;
			memset(elem_data, 0, elem_size);
			ReadMetadeskValue(tree, package, elem_data, &elem_type, NULL, child);
			i++;
		}
	}break;
	case HT_TypeKind_Int: {
		int* val = (int*)dst;
		bool ok = ParseMetadeskInt(value_node, val);
		assert(ok);
	}break;
	case HT_TypeKind_Float: {
		float* val = (float*)dst;
		bool ok = ParseMetadeskFloat(value_node, val);
		assert(ok);
	}break;
	case HT_TypeKind_AssetRef: {
		HT_AssetHandle* val = (HT_AssetHandle*)dst;
		Asset* found_asset = FindAssetFromPath(package, StrFromMD(value_node->string));
		*val = found_asset ? found_asset->handle : NULL;
	}break;
	default: TODO();
	}
}

static void ReloadAssetsPass3(ReloadAssetsContext* ctx, Asset* parent) {
	for (Asset* asset = parent->first_child; asset; asset = asset->next) {
		if (!asset->reload_assets_pass2_needs_hotreload) continue;

		MD_ParseResult parse;
		if (asset->kind == AssetKind_StructData || asset->kind == AssetKind_Plugin) {
			MD_String8 md_filepath = {(MD_u8*)asset->reload_assets_filesys_path.data, (MD_u64)asset->reload_assets_filesys_path.size};

			MD_ArenaClear(ctx->md_arena);
			parse = MD_ParseWholeFile(ctx->md_arena, md_filepath);
			assert(!MD_NodeIsNil(parse.node));
			assert(parse.errors.node_count == 0);
		}
		
		if (asset->kind == AssetKind_Plugin) {
			HT_Array* source_files = &asset->plugin.options.source_files;
			ArrayClear(source_files, sizeof(HT_AssetHandle));

			MD_Node* data_node = MD_ChildFromString(parse.node, MD_S8Lit("data_asset"), 0);
			
			Asset* data_asset = NULL;
			if (!MD_NodeIsNil(data_node)) {
				data_asset = FindAssetFromPath(ctx->package, StrFromMD(data_node->first_child->string));
			}
			asset->plugin.options.data = data_asset ? data_asset->handle : NULL;

			MD_Node* source_files_node = MD_ChildFromString(parse.node, MD_S8Lit("source_files"), 0);
			if (!MD_NodeIsNil(source_files_node)) {
				for (MD_EachNode(it, source_files_node->first_child)) {
					STR_View path = StrFromMD(it->string);
					Asset* elem = FindAssetFromPath(ctx->package, path);
					HT_AssetHandle elem_ref = elem ? elem->handle : NULL;

					ArrayPush(source_files, sizeof(HT_AssetHandle));
					((HT_AssetHandle*)source_files->data)[source_files->count - 1] = elem_ref;
				}
			}
		}

		if (asset->kind == AssetKind_StructData) {
			MD_Node* type_node = MD_ChildFromString(parse.node, MD_S8Lit("type"), 0);
			Asset* type_asset = FindAssetFromPath(ctx->package, StrFromMD(type_node->first_child->string));
			assert(type_asset != NULL);
			assert(type_asset->kind == AssetKind_StructType);
			
			DeinitStructDataAssetIfInitialized(asset);
			InitStructDataAsset(asset, type_asset);

			MD_Node* data_node = MD_ChildFromString(parse.node, MD_S8Lit("data"), 0);
			for (int i = 0; i < type_asset->struct_type.members.count; i++) {
				StructMember member = type_asset->struct_type.members[i];
				STR_View member_name = UI_TextToStr(member.name.text);
				
				MD_Node* member_node = MD_ChildFromString(data_node, StrToMD(member_name), 0);
				assert(!MD_NodeIsNil(member_node));
				
				ReadMetadeskValue(ctx->tree, ctx->package, (char*)asset->struct_data.data + member.offset, &member.type, member_node, member_node->first_child);
			}
		}

		ReloadAssetsPass3(ctx, asset);
	}
}

static void ReloadPackage(AssetTree* tree, Asset* package) {
	// We do loading in two passes.
	// 1. pass: delete assets which don't exist in the filesystem and make empty assets for those which do exist and we don't have as assets yet
	// 2. pass: per each asset, fully reload its contents from disk.
	// Two passes, because assets may refer to each other via asset paths in the serialized representation,
	// but in runtime representation those need to be resolved into asset handles. We must be able to refer
	// to other not-yet-loaded assets when loading an asset.
	// 3. pass: load struct data assets. This needs to be done as a separate pass AFTER all struct types have been loaded.
	
	// LogPrint(STR_(" *RELOADING PACKAGE *"));
	
	OS_SetWorkingDir(MEM_TEMP(), package->package.filesys_path);
	
	ReloadAssetsContext ctx = {0};
	ctx.tree = tree;
	ctx.package = package;
	ctx.md_arena = MD_ArenaAlloc();
	DS_ArrInit(&ctx.queue_recompile_plugins, TEMP);

	ReloadAssetsPass1(tree, package, package, package->package.filesys_path);
	
	ReloadAssetsPass2(&ctx, package);
	
	ReloadAssetsPass3(&ctx, package);
	
	//for (int i = 0; i < ctx.queue_recompile_plugins.length; i++) {
	//	ReloadPlugin(ctx.queue_recompile_plugins.data[i]);
	//}
	
	MD_ArenaRelease(ctx.md_arena);
}

EXPORT Asset* LoadPackageFromDisk(AssetTree* tree, STR_View path) {
	assert(OS_PathIsAbsolute(path));

	Asset* package = MakeNewAsset(tree, AssetKind_Package);
	package->ui_state_is_open = true;

	MoveAssetToInside(tree, package, tree->root);
	
	package->package.filesys_path = STR_Clone(DS_HEAP, path);
	
	bool ok = OS_InitDirectoryWatch(MEM_TEMP(), &package->package.dir_watch, package->package.filesys_path);
	assert(ok);
	
	ReloadPackage(tree, package);

	return package;
}

EXPORT void HotreloadPackages(AssetTree* tree) {
	for (Asset* asset = tree->root->first_child; asset; asset = asset->next) {
		if (asset->kind != AssetKind_Package) continue;

		if (OS_DirectoryWatchHasChanges(&asset->package.dir_watch)) {
			// printf("RELOADING PACKAGE!\n");
			ReloadPackage(tree, asset);
		}
	}
}

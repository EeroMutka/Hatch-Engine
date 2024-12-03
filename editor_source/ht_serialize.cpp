#define _CRT_SECURE_NO_WARNINGS // for fopen

#include "include/ht_common.h"

#define MD_FUNCTION extern "C"
#include "third_party/md.h"

static inline STR_View StrFromMD(MD_String8 str) { return {(char*)str.str, str.size}; }
static inline MD_String8 StrToMD(STR_View str) { return {(MD_u8*)str.data, (MD_u64)str.size}; }

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

static void LoadEditorPanel(EditorState* s, MD_Node* node, UI_Panel* panel) {
	UI_Axis split_along = -1;
	if (MD_S8Match(node->string, MD_S8Lit("split_h"), 0)) split_along = UI_Axis_X;
	if (MD_S8Match(node->string, MD_S8Lit("split_v"), 0)) split_along = UI_Axis_Y;
	
	if (split_along != -1) {
		panel->split_along = split_along;

		for (MD_Node* child = node->first_child; !MD_NodeIsNil(child); child = child->next) {
			UI_Panel* new_panel = NewUIPanel(&s->panel_tree);
			
			LoadEditorPanel(s, child, new_panel);

			UI_Panel* prev = panel->end_child[1];

			// push a new panel
			new_panel->parent = panel;
			new_panel->link[0] = panel->end_child[1];
			new_panel->link[1] = NULL;
			if (prev) prev->link[1] = new_panel;
			else panel->end_child[0] = new_panel;
			panel->end_child[1] = new_panel;
		}
	}
	else if (MD_S8Match(node->string, MD_S8Lit("log"), 0))            DS_ArrPush(&panel->tabs, s->log_tab_class);
	else if (MD_S8Match(node->string, MD_S8Lit("errors"), 0))         DS_ArrPush(&panel->tabs, s->errors_tab_class);
	else if (MD_S8Match(node->string, MD_S8Lit("assets"), 0))         DS_ArrPush(&panel->tabs, s->assets_tab_class);
	else if (MD_S8Match(node->string, MD_S8Lit("properties"), 0))     DS_ArrPush(&panel->tabs, s->properties_tab_class);
	else if (MD_S8Match(node->string, MD_S8Lit("asset_viewer"), 0))   DS_ArrPush(&panel->tabs, s->asset_viewer_tab_class);
}

EXPORT void RegenerateTypeTable(EditorState* s) {
	DS_ArrClear(&s->type_table);
	for (DS_BkArrEach(&s->asset_tree.assets, asset_i)) {
		Asset* asset = DS_BkArrGet(&s->asset_tree.assets, asset_i);
		if (asset->kind == AssetKind_StructType) {
			STR_View name = UI_TextToStr(asset->name);
			if (STR_Match(name, "Untitled Struct")) continue; // temporary hack against builtin structures
			
			DS_ArrPush(&s->type_table, asset->handle);
		}
	}
	s->api->types = (HT_GeneratedTypeTable*)s->type_table.data;
}

EXPORT void LoadProject(EditorState* s, STR_View project_directory) {
	ASSERT(OS_PathIsAbsolute(project_directory));

	MD_Arena* md_arena = MD_ArenaAlloc();
	
	MD_String8 md_filepath = StrToMD(STR_Form(TEMP, "%v/.htproject", project_directory));
	MD_ParseResult parse = MD_ParseWholeFile(md_arena, md_filepath);
	ASSERT(!MD_NodeIsNil(parse.node));
	ASSERT(parse.errors.node_count == 0);
	
	MD_Node* packages = MD_ChildFromString(parse.node, MD_S8Lit("packages"), 0);
	if (!MD_NodeIsNil(packages)) {
		DS_DynArray(STR_View) package_paths = {TEMP};

		for (MD_Node* child = packages->first_child; !MD_NodeIsNil(child); child = child->next) {
			STR_View path = StrFromMD(child->string);
			if (STR_CutStart(&path, "$HATCH_DIR")) {
				path = STR_Form(TEMP, "%s%v", HATCH_DIR, path);
			}
			DS_ArrPush(&package_paths, path);
		}
		
		LoadPackages(s, package_paths);
	}
	
	RegenerateTypeTable(s);

	MD_Node* editor_layout = MD_ChildFromString(parse.node, MD_S8Lit("editor_layout"), 0);
	if (!MD_NodeIsNil(editor_layout)) {
		ASSERT(editor_layout->first_child == editor_layout->last_child && !MD_NodeIsNil(editor_layout->first_child));
		LoadEditorPanel(s, editor_layout->first_child, s->panel_tree.root);
	}

	MD_Node* run_plugins = MD_ChildFromString(parse.node, MD_S8Lit("run_plugins"), 0);
	if (!MD_NodeIsNil(run_plugins)) {
		for (MD_Node* child = run_plugins->first_child; !MD_NodeIsNil(child); child = child->next) {
			STR_View path = StrFromMD(child->string);
			Asset* plugin_asset = FindAssetFromPath(&s->asset_tree, NULL, path);
			ASSERT(plugin_asset->kind == AssetKind_Plugin);
			
			plugin_asset->plugin.active_by_request = true;
			RunPlugin(s, plugin_asset);
		}
	}

	MD_ArenaRelease(md_arena);
}

EXPORT STR_View AssetGetPackageRelativePath(DS_Arena* arena, Asset* asset) {
	if (asset->kind == AssetKind_Package) return "";

	STR_View result = AssetGetFilename(TEMP, asset);
	for (Asset* p = asset->parent; p->kind != AssetKind_Package; p = p->parent) {
		result = STR_Form(TEMP, "%v/%v", UI_TextToStr(p->name), result);
	}
	return result;
}

EXPORT STR_View AssetGetAbsoluteFilepath(DS_Arena* arena, Asset* asset) {
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

struct ReloadAssetsContext {
	AssetTree* tree;
	MD_Arena* md_arena;
	DS_DynArray(Asset*) queue_recompile_plugins;
};

// While we go, we also generate and update filesys_paths.
static void SaveAsset(Asset* asset, STR_View filesys_path) {
	ASSERT(!STR_ContainsU(filesys_path, '\\'));

	if (asset->kind == AssetKind_Folder || asset->kind == AssetKind_Package) {
		bool ok = OS_MakeDirectory(MEM_SCOPE_NONE, filesys_path);
		ASSERT(ok);

		OS_FileInfoArray files;
		ok = OS_GetAllFilesInDirectory(MEM_SCOPE_TEMP, filesys_path, &files);

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
					OS_DeleteDirectory(MEM_SCOPE_NONE, info->name);
				} else {
					OS_DeleteFile(MEM_SCOPE_NONE, info->name);
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
			bool file_exists = OS_FileGetModtime(MEM_SCOPE_NONE, filesys_path, &modtime);
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
				fprintf(file, "code_files: {\n");
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
	ASSERT(package->kind == AssetKind_Package);

	if (package->package.filesys_path.size == 0) {
		STR_View filesys_path;
		bool ok = OS_FolderPicker(MEM_SCOPE_TEMP, &filesys_path);
		if (!ok) {
			printf("Invalid path selected!\n"); // TODO: use log window
			return;
		}
		package->package.filesys_path = STR_Clone(DS_HEAP, filesys_path);
	}

	OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);

	SaveAsset(package, package->package.filesys_path);

	OS_SetWorkingDir(MEM_SCOPE_NONE, DEFAULT_WORKING_DIRECTORY); // reset working directory
}

static void ReloadAssetsPass1(AssetTree* tree, Asset* parent, STR_View parent_full_path) {
	OS_FileInfoArray files = {0};
	OS_GetAllFilesInDirectory(MEM_SCOPE_TEMP, parent_full_path, &files);

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

		ASSERT(info.last_write_time >= asset->modtime); // modtime should never decrease on windows
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
			ReloadAssetsPass1(tree, asset, full_path);
		}
	}
}

struct MDParser {
	MD_Node* node;
};

static bool ParseMetadeskInt(MDParser* p, int *out_value) {
	int sign = 1;
	if (p->node->flags & MD_NodeFlag_Symbol && MD_S8Match(p->node->string, MD_S8Lit("-"), 0)) {
		p->node = p->node->next;
		sign = -1;
	}
	if (MD_NodeIsNil(p->node)) return false;
	if (!(p->node->flags & MD_NodeFlag_Numeric)) return false;
	if (!MD_StringIsCStyleInt(p->node->string)) return false;

	*out_value = sign * (int)MD_CStyleIntFromString(p->node->string);

	p->node = p->node->next;
	return true;
}

static bool ParseMetadeskFloat(MDParser* p, float *out_value) {
	float sign = 1.f;
	if (p->node->flags & MD_NodeFlag_Symbol && MD_S8Match(p->node->string, MD_S8Lit("-"), 0)) {
		p->node = p->node->next;
		sign = -1.f;
	}
	if (MD_NodeIsNil(p->node)) return false;
	if (!(p->node->flags & MD_NodeFlag_Numeric)) return false;

	if (MD_StringIsCStyleInt(p->node->string)) {
		*out_value = sign * (float)MD_CStyleIntFromString(p->node->string);
	}
	else {
		double value;
		if (!STR_ParseFloat(StrFromMD(p->node->string), &value)) return false;
		*out_value = sign * (float)value;
	}

	p->node = p->node->next;
	return true;
}

static HT_Type ParseMetadeskType(AssetTree* tree, Asset* package, MDParser* p) {
	ASSERT(!MD_NodeIsNil(p->node));

	HT_Type result = {};
	STR_View type_name = StrFromMD(p->node->string);

	HT_TypeKind builtin_type = StringToTypeKind(type_name);
	if (builtin_type != HT_TypeKind_INVALID) {
		result.kind = builtin_type;
	}
	else {
		// Must be a user-defined type
		Asset* struct_type = FindAssetFromPath(tree, package, type_name);
		ASSERT(struct_type);
		result.kind = HT_TypeKind_Struct;
		result.handle = struct_type->handle;
	}

	if (MD_NodeHasTag(p->node, MD_S8Lit("Array"), 0)) {
		result.subkind = result.kind;
		result.kind = HT_TypeKind_Array;
	}

	if (MD_NodeHasTag(p->node, MD_S8Lit("ItemGroup"), 0)) {
		result.subkind = result.kind;
		result.kind = HT_TypeKind_ItemGroup;
	}

	p->node = p->node->next;
	return result;
}

static void ReloadAssetsPass2(ReloadAssetsContext* ctx, Asset* package, Asset* parent) {
	for (Asset* asset = parent->first_child; asset; asset = asset->next) {
		if (asset->reload_assets_pass2_needs_hotreload) {
			MD_ParseResult parse;
			if (asset->kind == AssetKind_StructType)
			{
				MD_String8 md_filepath = {(MD_u8*)asset->reload_assets_filesys_path.data, (MD_u64)asset->reload_assets_filesys_path.size};

				MD_ArenaClear(ctx->md_arena);
				parse = MD_ParseWholeFile(ctx->md_arena, md_filepath);
				ASSERT(!MD_NodeIsNil(parse.node));
				ASSERT(parse.errors.node_count == 0);
			}

			if (asset->kind == AssetKind_StructType) {
				MD_Node* struct_node = MD_ChildFromString(parse.node, MD_S8Lit("struct"), 0);
				for (int i = 0; i < asset->struct_type.members.count; i++) {
					StructMemberDeinit(&asset->struct_type.members[i]);
				}
				DS_ArrClear(&asset->struct_type.members);

				//DS_ArrClear(&asset->struct_members);
				ASSERT(!MD_NodeIsNil(struct_node));
				for (MD_EachNode(it, struct_node->first_child)) {
					StructMember member = {0};
					StructMemberInit(&member);

					STR_View name = StrFromMD(it->string);
					UI_TextSet(&member.name.text, name);

					MDParser child_p = {it->first_child};
					member.type = ParseMetadeskType(ctx->tree, package, &child_p);

					DS_ArrPush(&asset->struct_type.members, member);
				}

				ComputeStructLayout(ctx->tree, asset);
			}
		}

		ReloadAssetsPass2(ctx, package, asset);
	}
}

// `dst` is expected to be zero-initialized.
static void ParseMetadeskValue(AssetTree* tree, Asset* package, void* dst, HT_Type* type, MDParser* p) {
	// if the child is an array, struct or ItemGroup, then `node` is actually the first child in that container. Otherwise it's the value node itself.

	switch (type->kind) {
	case HT_TypeKind_Struct: {
		Asset* struct_asset = GetAsset(tree, type->handle);

		for (int i = 0; i < struct_asset->struct_type.members.count; i++) {
			StructMember member = struct_asset->struct_type.members[i];
			STR_View member_name = UI_TextToStr(member.name.text);

			ASSERT(!MD_NodeIsNil(p->node));
			ASSERT(MD_S8Match(p->node->string, StrToMD(member_name), 0));

			MDParser child_p = {p->node->first_child};
			ParseMetadeskValue(tree, package, (char*)dst + member.offset, &member.type, &child_p);
			p->node = p->node->next;
		}
	}break;
	case HT_TypeKind_ItemGroup: {
		HT_ItemGroup* val = (HT_ItemGroup*)dst;

		HT_Type item_type = *type;
		item_type.kind = type->subkind;

		for (; !MD_NodeIsNil(p->node); p->node = p->node->next) {
			HT_ItemIndex item_i = ItemGroupAdd(val);

			HT_ItemHeader* item = GetItemFromIndex(val, item_i);
			STR_View name = STR_Clone(DS_HEAP, StrFromMD(p->node->string));
			item->name = {name.data, name.size};

			MoveItemToAfter(val, item_i, val->last);

			void* item_data = (char*)GetItemFromIndex(val, item_i) + val->item_offset;
			Construct(tree, item_data, &item_type);

			MDParser child_p = {p->node->first_child};
			ParseMetadeskValue(tree, package, item_data, &item_type, &child_p);
		}
	}break;
	case HT_TypeKind_Array: {
		HT_Array* val = (HT_Array*)dst;

		HT_Type elem_type = *type;
		elem_type.kind = type->subkind;

		i32 elem_size, elem_align;
		GetTypeSizeAndAlignment(tree, &elem_type, &elem_size, &elem_align);

		int i = 0;
		while (!MD_NodeIsNil(p->node)) {
			ArrayPush(val, elem_size);
			char* elem_data = (char*)val->data + elem_size*i;
			Construct(tree, elem_data, &elem_type);
			ParseMetadeskValue(tree, package, elem_data, &elem_type, p);
			i++;
		}
	}break;
	case HT_TypeKind_Int: {
		int* val = (int*)dst;
		bool ok = ParseMetadeskInt(p, val);
		ASSERT(ok);
	}break;
	case HT_TypeKind_Float: {
		float* val = (float*)dst;
		bool ok = ParseMetadeskFloat(p, val);
		ASSERT(ok);
	}break;
	case HT_TypeKind_Vec2: TODO(); break;
	case HT_TypeKind_Vec3: {
		for (int i = 0; i < 3; i++) {
			ASSERT(!MD_NodeIsNil(p->node));
			bool ok = ParseMetadeskFloat(p, &((float*)dst)[i]);
			ASSERT(ok);
		}
	}break;
	case HT_TypeKind_Vec4: TODO(); break;
	case HT_TypeKind_IVec2: TODO(); break;
	case HT_TypeKind_IVec3: TODO(); break;
	case HT_TypeKind_IVec4: TODO(); break;
	case HT_TypeKind_Bool: {
		bool* val = (bool*)dst;
		/**/ if (MD_S8Match(p->node->string, MD_S8Lit("true"), 0)) *val = true;
		else if (MD_S8Match(p->node->string, MD_S8Lit("false"), 0)) *val = false;
		else ASSERT(0);
		p->node = p->node->next;
	}break;
	case HT_TypeKind_String: {
		TODO();
	}break;
	case HT_TypeKind_Type: {
		TODO();
	}break;
	case HT_TypeKind_Any: {
		HT_Any* val = (HT_Any*)dst;
		MD_Node* type_tag = MD_TagFromString(p->node, MD_S8Lit("Type"), 0);
		ASSERT(!MD_NodeIsNil(type_tag));

		MDParser type_tag_child_p = {type_tag->first_child};
		HT_Type type = ParseMetadeskType(tree, package, &type_tag_child_p);
		AnyChangeType(tree, val, &type);

		MDParser child_p = {p->node->first_child};
		ParseMetadeskValue(tree, package, val->data, &type, &child_p);
		p->node = p->node->next;
	}break;
	case HT_TypeKind_AssetRef: {
		HT_Asset* val = (HT_Asset*)dst;
		Asset* found_asset = FindAssetFromPath(tree, package, StrFromMD(p->node->string));
		*val = found_asset ? found_asset->handle : NULL;
		p->node = p->node->next;
	}break;
	case HT_TypeKind_COUNT: ASSERT(0); break;
	case HT_TypeKind_INVALID: ASSERT(0); break;
	}
}

static void ReloadAssetsPass3(ReloadAssetsContext* ctx, Asset* package, Asset* parent) {
	for (Asset* asset = parent->first_child; asset; asset = asset->next) {
		if (asset->reload_assets_pass2_needs_hotreload) {
			MD_ParseResult parse;
			if (asset->kind == AssetKind_StructData || asset->kind == AssetKind_Plugin) {
				MD_String8 md_filepath = {(MD_u8*)asset->reload_assets_filesys_path.data, (MD_u64)asset->reload_assets_filesys_path.size};

				MD_ArenaClear(ctx->md_arena);
				parse = MD_ParseWholeFile(ctx->md_arena, md_filepath);
				ASSERT(!MD_NodeIsNil(parse.node));
				ASSERT(parse.errors.node_count == 0);
			}

			if (asset->kind == AssetKind_Plugin) {
				HT_Array* code_files = &asset->plugin.options.code_files;
				ArrayClear(code_files, sizeof(HT_Asset));

				MD_Node* data_node = MD_ChildFromString(parse.node, MD_S8Lit("data_asset"), 0);

				Asset* data_asset = NULL;
				if (!MD_NodeIsNil(data_node)) {
					data_asset = FindAssetFromPath(ctx->tree, package, StrFromMD(data_node->first_child->string));
				}
				asset->plugin.options.data = data_asset ? data_asset->handle : NULL;

				MD_Node* source_files_node = MD_ChildFromString(parse.node, MD_S8Lit("code_files"), 0);
				if (!MD_NodeIsNil(source_files_node)) {
					for (MD_EachNode(it, source_files_node->first_child)) {
						STR_View path = StrFromMD(it->string);
						Asset* elem = FindAssetFromPath(ctx->tree, package, path);
						HT_Asset elem_ref = elem ? elem->handle : NULL;

						ArrayPush(code_files, sizeof(HT_Asset));
						((HT_Asset*)code_files->data)[code_files->count - 1] = elem_ref;
					}
				}
			}

			if (asset->kind == AssetKind_StructData) {
				MD_Node* type_node = MD_ChildFromString(parse.node, MD_S8Lit("type"), 0);
				Asset* type_asset = FindAssetFromPath(ctx->tree, package, StrFromMD(type_node->first_child->string));
				ASSERT(type_asset != NULL);
				ASSERT(type_asset->kind == AssetKind_StructType);

				DeinitStructDataAssetIfInitialized(ctx->tree, asset);
				InitStructDataAsset(ctx->tree, asset, type_asset);

				MD_Node* data_node = MD_ChildFromString(parse.node, MD_S8Lit("data"), 0);

				HT_Type type = { HT_TypeKind_Struct };
				type.handle = type_asset->handle;
				MDParser child_p = {data_node->first_child};
				ParseMetadeskValue(ctx->tree, package, asset->struct_data.data, &type, &child_p);
			}
		}

		// queue plugin for recompilation if any of its source files has changed
		if (asset->kind == AssetKind_Plugin) {
			bool code_file_was_hotreloaded = false;

			HT_Array code_files = asset->plugin.options.code_files;
			for (int i = 0; i < code_files.count; i++) {
				HT_Asset code_file_handle = ((HT_Asset*)code_files.data)[i];
				Asset* code_file = GetAsset(ctx->tree, code_file_handle);
				if (code_file && code_file->reload_assets_pass2_needs_hotreload) {
					code_file_was_hotreloaded = true;
					break;
				}
			}

			if (code_file_was_hotreloaded) {
				DS_ArrPush(&ctx->queue_recompile_plugins, asset);
			}
		}

		ReloadAssetsPass3(ctx, package, asset);
	}
}

static void ReloadPackages(EditorState* s, DS_ArrayView<Asset*> packages) {
	// We do loading in two passes.
	// 1. pass: delete assets which don't exist in the filesystem and make empty assets for those which do exist and we don't have as assets yet
	// 2. pass: per each asset, fully reload its contents from disk.
	// Two passes, because assets may refer to each other via asset paths in the serialized representation,
	// but in runtime representation those need to be resolved into asset handles. We must be able to refer
	// to other not-yet-loaded assets when loading an asset.
	// 3. pass: load struct data assets. This needs to be done as a separate pass AFTER all struct types have been loaded.

	ReloadAssetsContext ctx = {0};
	ctx.tree = &s->asset_tree;
	ctx.md_arena = MD_ArenaAlloc();
	DS_ArrInit(&ctx.queue_recompile_plugins, TEMP);

	for (int i = 0; i < packages.count; i++) {
		Asset* package = packages[i];
		OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);
		ReloadAssetsPass1(&s->asset_tree, package, package->package.filesys_path);
	}

	for (int i = 0; i < packages.count; i++) {
		Asset* package = packages[i];
		OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);
		ReloadAssetsPass2(&ctx, package, package);
	}

	for (int i = 0; i < packages.count; i++) {
		Asset* package = packages[i];
		OS_SetWorkingDir(MEM_SCOPE_NONE, package->package.filesys_path);
		ReloadAssetsPass3(&ctx, package, package);
	}

#ifdef HT_DYNAMIC
	for (int i = 0; i < ctx.queue_recompile_plugins.count; i++) {
		Asset* plugin_asset = ctx.queue_recompile_plugins[i];

		if (plugin_asset->plugin.active_instance != NULL) {
			UnloadPlugin(s, plugin_asset);
		}
		
		RecompilePlugin(s, plugin_asset);
		
		if (plugin_asset->plugin.active_by_request) {
			RunPlugin(s, plugin_asset);
		}

		printf("Recompiling plugin!\n");
	}
	if (ctx.queue_recompile_plugins.count > 0) {
		RegenerateTypeTable(s);
	}
#endif

	MD_ArenaRelease(ctx.md_arena);
	OS_SetWorkingDir(MEM_SCOPE_NONE, DEFAULT_WORKING_DIRECTORY); // reset working directory
}

EXPORT void LoadPackages(EditorState* s, DS_ArrayView<STR_View> paths) {
	DS_DynArray(Asset*) packages = {TEMP};

	for (int i = 0; i < paths.count; i++) {
		STR_View path = paths[i];
		ASSERT(OS_PathIsAbsolute(path));

		Asset* package = MakeNewAsset(&s->asset_tree, AssetKind_Package);
		package->ui_state_is_open = true;

		STR_View package_name = STR_AfterLast(path, '/');
		bool has_dollar = STR_CutStart(&package_name, "$");
		ASSERT(has_dollar);

		u64 name_hash = DS_MurmurHash64A(package_name.data, package_name.size, 0);
		bool newly_added = DS_MapInsert(&s->asset_tree.package_from_name, name_hash, package);
		ASSERT(newly_added);

		MoveAssetToInside(&s->asset_tree, package, s->asset_tree.root);

		package->package.filesys_path = STR_Clone(DS_HEAP, path);

		bool ok = OS_InitDirectoryWatch(MEM_SCOPE_NONE, &package->package.dir_watch, package->package.filesys_path);
		ASSERT(ok);

		DS_ArrPush(&packages, package);
	}
	
	ReloadPackages(s, packages);
}

EXPORT void HotreloadPackages(EditorState* s) {
	for (Asset* asset = s->asset_tree.root->first_child; asset; asset = asset->next) {
		if (asset->kind != AssetKind_Package) continue;

		if (OS_DirectoryWatchHasChanges(&asset->package.dir_watch)) {
			printf("RELOADING PACKAGE!\n");
			ReloadPackages(s, {&asset, 1});
		}
	}
}

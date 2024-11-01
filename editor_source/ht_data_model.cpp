#include "include/ht_internal.h"

EXPORT STR_View TypeKindToString(TypeKind type) {
	switch (type) {
	case TypeKind_Float:     return "Float";
	case TypeKind_Int:       return "Int";
	case TypeKind_Bool:      return "Bool";
	case TypeKind_AssetRef:  return "Asset";
	case TypeKind_Struct:    return "Struct";
	case TypeKind_Array:     return "Array";
	case TypeKind_String:    return "String";
	case TypeKind_Type:      return "Type";
	case TypeKind_COUNT:   break;
	case TypeKind_INVALID: break;
	}
	return "(invalid)";
}

EXPORT TypeKind StringToTypeKind(STR_View str) {
	/**/ if (STR_Match(str, "Float"))  return TypeKind_Float;
	else if (STR_Match(str, "Int"))    return TypeKind_Int;
	else if (STR_Match(str, "Bool"))   return TypeKind_Bool;
	else if (STR_Match(str, "Asset"))  return TypeKind_AssetRef;
	else if (STR_Match(str, "Struct")) return TypeKind_Struct;
	else if (STR_Match(str, "Array"))  return TypeKind_Array;
	else if (STR_Match(str, "String")) return TypeKind_String;
	else if (STR_Match(str, "Type"))   return TypeKind_Type;
	return TypeKind_INVALID;
}

EXPORT void MoveAssetToBefore(AssetTree* tree, Asset* asset, Asset* move_before_this) {
	assert(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

	Asset* parent = move_before_this ? move_before_this->parent : tree->root;
	Asset* prev = move_before_this ? move_before_this->prev : tree->root->last_child;
	Asset* next = move_before_this;
	
	asset->parent = parent;
	asset->prev = prev;
	asset->next = next;
	if (prev) prev->next = asset;
	else parent->first_child = asset;
	if (next) next->prev = asset;
	else parent->last_child = asset;
}

EXPORT void MoveAssetToAfter(AssetTree* tree, Asset* asset, Asset* move_after_this) {
	assert(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

	Asset* parent = move_after_this ? move_after_this->parent : tree->root;
	Asset* prev = move_after_this;
	Asset* next = move_after_this ? move_after_this->next : tree->root->first_child;

	asset->parent = parent;
	asset->prev = prev;
	asset->next = next;
	if (prev) prev->next = asset;
	else parent->first_child = asset;
	if (next) next->prev = asset;
	else parent->last_child = asset;
}

EXPORT void MoveAssetToInside(AssetTree* tree, Asset* asset, Asset* move_inside_this) {
	assert(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

	Asset* parent = move_inside_this;
	Asset* prev = move_inside_this->last_child;

	asset->parent = parent;
	asset->prev = prev;
	asset->next = NULL;
	if (prev) prev->next = asset;
	else parent->first_child = asset;
	parent->last_child = asset;
}

EXPORT bool AssetIsValid(AssetRef handle) {
	return handle.asset != NULL && handle.asset->generation == handle.generation;
}

EXPORT AssetRef GetAssetHandle(Asset* asset) {
	return {asset, asset->generation};
}

EXPORT Asset* MakeNewAsset(AssetTree* tree, AssetKind kind) {
	Asset* asset;
	NEW_SLOT(&asset, &tree->assets, &tree->first_free_asset, freelist_next);

	*asset = {};
	asset->kind = kind;
	asset->generation = tree->next_asset_generation++;

	STR_View name = "";
	switch (kind) {
	case AssetKind_Root: break;
	case AssetKind_Package: break;
	case AssetKind_Folder:  { name = "Untitled Folder"; }break;
	case AssetKind_Plugin: {
		//asset->plugin.options_data = DS_MemAlloc(DS_HEAP, tree->plugin_options_struct_type->struct_type.size);
		//memset(asset->plugin.options_data, 0, tree->plugin_options_struct_type->struct_type.size);
		name = "Untitled Plugin";
		DS_ArrInit(&asset->plugin.allocations, DS_HEAP);
	}break;
	case AssetKind_StructType: {
		DS_ArrInit(&asset->struct_type.members, DS_HEAP);
		name = "Untitled Struct";
	}break;
	case AssetKind_StructData: {
		name = "Untitled Data";
	}break;
	case AssetKind_File: {
		name = "Untitled.txt";
	}break;
	//case AssetKind_C: {
	//	name = "Untitled C File";
	//}break;
	//case AssetKind_CPP: {
	//	name = "Untitled CPP File";
	//}break;
	}
	if (kind != AssetKind_Package) {
		UI_TextInit(DS_HEAP, &asset->name, name);
	}

	return asset;
}

EXPORT void StructTypeAddMember(AssetTree* tree, Asset* struct_type) {
	// so we want to "rebuild" the struct datas. For that, we need a "lookup" into the old type  Making a destructive change to a type...!
	// I guess we could serialize and deserialize everything...
	// since we have an array, lets do the simplest thing and just array remove element/etc.

	StructMember member = {0};
	UI_TextInit(DS_HEAP, &member.name.text, "Unnamed member");
	DS_ArrPush(&struct_type->struct_type.members, member);
	ComputeStructLayout(struct_type);

	// Sync all struct assets data to the new type layout
	DS_ForBucketArrayEach(Asset, &tree->assets, IT) {
		if (IT.elem->kind != AssetKind_StructData || IT.elem->struct_data.struct_type.asset != struct_type) continue;

		TODO();
		// we want to also have a DS_Set per struct type that recursively references all struct types it depends on
	}
}

EXPORT void InitStructDataAsset(Asset* asset, Asset* struct_type) {
	assert(asset->kind == AssetKind_StructData);
	assert(struct_type->kind == AssetKind_StructType);

	asset->struct_data.struct_type = GetAssetHandle(struct_type);

	asset->struct_data.data = DS_MemAlloc(DS_HEAP, struct_type->struct_type.size);
	memset(asset->struct_data.data, 0, struct_type->struct_type.size);

	/*for (StructMemberNode* type_node = struct_type->struct_type.root->first_child; type_node; type_node = type_node->next) {
		StructDataMember member;
		memset(&member, 0, sizeof(member));
		DS_ArrPush(&asset->struct_data.members, member);
	}*/
}

EXPORT void DeinitStructDataAssetIfInitialized(Asset* asset) {
	if (asset->struct_data.data) {
		// TODO: free all arrays and other stuff!
		TODO();

		DS_MemFree(DS_HEAP, asset->struct_data.data);
		asset->struct_data.data = NULL;
	}
}

EXPORT void GetTypeSizeAndAlignment(Type* type, i32* out_size, i32* out_alignment) {
	switch (type->kind) {
	case TypeKind_Int:        { *out_size = 4; *out_alignment = 4; } return;
	case TypeKind_Float:      { *out_size = 4; *out_alignment = 4; } return;
	case TypeKind_Bool:       { *out_size = 1; *out_alignment = 1; } return;
	case TypeKind_AssetRef:   { *out_size = sizeof(AssetRef); *out_alignment = alignof(AssetRef); } return;
	case TypeKind_Array:      { *out_size = sizeof(Array); *out_alignment = alignof(Array); } return;
	case TypeKind_String:     { *out_size = sizeof(String); *out_alignment = alignof(String); } return;
	case TypeKind_Struct:     {
		assert(AssetIsValid(type->_struct) && type->_struct.asset->kind == AssetKind_StructType);
		Asset_StructType* struct_type = &type->_struct.asset->struct_type;
		*out_size = struct_type->size;
		*out_alignment = struct_type->alignment;
	} return;
	case TypeKind_Type:       { *out_size = sizeof(Type); *out_alignment = alignof(Type); } return;
	case TypeKind_COUNT: break;
	case TypeKind_INVALID: break;
	}
}

EXPORT void ComputeStructLayout(Asset* struct_type) {
	assert(struct_type->kind == AssetKind_StructType);
	i32 offset = 0;
	i32 max_alignment = 1;
	
	for (int i = 0; i < struct_type->struct_type.members.count; i++) {
		StructMember* member = &struct_type->struct_type.members[i];
		i32 member_size = 0, member_alignment = 0;
		GetTypeSizeAndAlignment(&member->type, &member_size, &member_alignment);
		
		max_alignment = UI_Max(max_alignment, member_alignment);
		
		offset = DS_AlignUpPow2(offset, member_alignment);
		member->offset = offset;
		offset += member_size;
	}

	struct_type->struct_type.size = DS_AlignUpPow2(offset, max_alignment);
	struct_type->struct_type.alignment = max_alignment;
}

EXPORT void ArrayPush(Array* array, int32_t elem_size) {
	if (array->count == array->capacity) {
		int32_t new_capacity = array->capacity == 0 ? 8 : array->capacity * 2;
		array->data = DS_MemResize(DS_HEAP, array->data, array->capacity * elem_size, new_capacity * elem_size);
		array->capacity = new_capacity;
		//if (array->capacity == 0) {
		//	int32_t new_size = elem_size * 8;
		//	array->data = DS_MemAlloc(DS_HEAP, new_size);
		//	array->capacity = 8;
		//}
		//else {
		//	int32_t new_capacity = array->capacity * 2;
		//	int32_t old_size = array->capacity * elem_size;
		//	int32_t new_size = new_capacity * elem_size;
		//	array->data = DS_AllocatorFn(DS_HEAP, array->data, old_size, new_size, DS_DEFAULT_ALIGNMENT);
		//	array->capacity = new_capacity;
		//}
	}
	memset((char*)array->data + array->count * elem_size, 0, elem_size);
	array->count++;
}

EXPORT void StructMemberInit(StructMember* x) {
	StringInit(&x->name);
}
EXPORT void StructMemberDeinit(StructMember* x) {
	StringDeinit(&x->name);
}

EXPORT void StringInit(String* x) { UI_TextInit(DS_HEAP, &x->text, ""); }
EXPORT void StringDeinit(String* x) { UI_TextDeinit(&x->text); }

EXPORT void ArrayClear(Array* array, int32_t elem_size) {
	array->count = 0;
	// TODO: We should shrink the array by default.
}

EXPORT void DeleteAssetIncludingChildren(AssetTree* tree, Asset* asset) {
	for (Asset* child = asset->first_child; child;) {
		Asset* next = child->next;
		DeleteAssetIncludingChildren(tree, child);
		child = next;
	}

	// Remove from the tree
	if (asset->prev) asset->prev->next = asset->next;
	else asset->parent->first_child = asset->next;
	if (asset->next) asset->next->prev = asset->prev;
	else asset->parent->last_child = asset->prev;

	if (asset->kind != AssetKind_Package) {
		UI_TextDeinit(&asset->name);
	}

	switch (asset->kind) {
	case AssetKind_Root: break;
	case AssetKind_Package: {
		OS_DeinitDirectoryWatch(&asset->package.dir_watch);
		STR_Free(DS_HEAP, asset->package.filesys_path);
	}break;
	case AssetKind_Folder: {
	}break;
	//case AssetKind_C: {}break;
	//case AssetKind_CPP: {}break;
	case AssetKind_File: {}break;
	case AssetKind_Plugin: {
		//DS_MemFree(DS_HEAP, asset->plugin.options_data);
		DS_ArrDeinit(&asset->plugin.allocations);
	}break;
	case AssetKind_StructType: {
		DS_ArrDeinit(&asset->struct_type.members);
	}break;
	case AssetKind_StructData: {
		DeinitStructDataAssetIfInitialized(asset);
	}break;
	}

	asset->kind = AssetKind_FreeSlot;
	asset->freelist_next = tree->first_free_asset;
	tree->first_free_asset = asset;
}

EXPORT Asset* FindAssetFromPath(Asset* package, STR_View path) {
	Asset* parent = package;
	Asset* result = NULL;

	for (STR_View remaining = path; remaining.size > 0 && parent;) {
		STR_View name = STR_ParseUntilAndSkip(&remaining, '/');
		Asset* new_parent = NULL;
		for (Asset* asset = parent->first_child; asset; asset = asset->next) {
			if (STR_MatchCaseInsensitive(UI_TextToStr(asset->name), name)) {
				new_parent = asset;
				break;
			}
		}
		parent = new_parent;
		result = parent;
	}

	return result;
}
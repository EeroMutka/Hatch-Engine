#include "include/ht_common.h"

EXPORT STR_View HT_TypeKindToString(HT_TypeKind type) {
	switch (type) {
	case HT_TypeKind_Float:     return "Float";
	case HT_TypeKind_Int:       return "Int";
	case HT_TypeKind_Bool:      return "Bool";
	case HT_TypeKind_Vec2:      return "Vec2";
	case HT_TypeKind_Vec3:      return "Vec3";
	case HT_TypeKind_Vec4:      return "Vec4";
	case HT_TypeKind_IVec2:     return "IVec2";
	case HT_TypeKind_IVec3:     return "IVec3";
	case HT_TypeKind_IVec4:     return "IVec4";
	case HT_TypeKind_AssetRef:  return "Asset";
	case HT_TypeKind_Struct:    return "Struct";
	case HT_TypeKind_Array:     return "Array";
	case HT_TypeKind_Any:       return "Any";
	case HT_TypeKind_ItemGroup: return "ItemGroup";
	case HT_TypeKind_String:    return "String";
	case HT_TypeKind_Type:      return "HT_Type";
	case HT_TypeKind_COUNT:   break;
	case HT_TypeKind_INVALID: break;
	}
	return "(invalid)";
}

EXPORT HT_TypeKind StringToTypeKind(STR_View str) {
	/**/ if (STR_Match(str, "Float"))     return HT_TypeKind_Float;
	else if (STR_Match(str, "Int"))       return HT_TypeKind_Int;
	else if (STR_Match(str, "Bool"))      return HT_TypeKind_Bool;
	else if (STR_Match(str, "Vec2"))      return HT_TypeKind_Vec2;
	else if (STR_Match(str, "Vec3"))      return HT_TypeKind_Vec3;
	else if (STR_Match(str, "Vec4"))      return HT_TypeKind_Vec4;
	else if (STR_Match(str, "IVec2"))     return HT_TypeKind_IVec2;
	else if (STR_Match(str, "IVec3"))     return HT_TypeKind_IVec3;
	else if (STR_Match(str, "IVec4"))     return HT_TypeKind_IVec4;
	else if (STR_Match(str, "Asset"))     return HT_TypeKind_AssetRef;
	else if (STR_Match(str, "Struct"))    return HT_TypeKind_Struct;
	else if (STR_Match(str, "Array"))     return HT_TypeKind_Array;
	else if (STR_Match(str, "Any"))       return HT_TypeKind_Any;
	else if (STR_Match(str, "ItemGroup")) return HT_TypeKind_ItemGroup;
	else if (STR_Match(str, "String"))    return HT_TypeKind_String;
	else if (STR_Match(str, "HT_Type"))   return HT_TypeKind_Type;
	return HT_TypeKind_INVALID;
}

EXPORT void MoveAssetToBefore(AssetTree* tree, Asset* asset, Asset* move_before_this) {
	ASSERT(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

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
	ASSERT(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

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
	ASSERT(asset->parent == NULL); // For now, assert that the assert is not attached to the tree

	Asset* parent = move_inside_this;
	Asset* prev = move_inside_this->last_child;

	asset->parent = parent;
	asset->prev = prev;
	asset->next = NULL;
	if (prev) prev->next = asset;
	else parent->first_child = asset;
	parent->last_child = asset;
}

EXPORT bool AssetIsValid(AssetTree* tree, HT_Asset handle) {
	return GetAsset(tree, handle) != NULL;
}

EXPORT Asset* GetAsset(AssetTree* tree, HT_Asset handle) {
	AssetHandleDecoded decoded = *(AssetHandleDecoded*)&handle;
	if (decoded.bucket_index < tree->asset_buckets.count) {
		AssetBucket* bucket = tree->asset_buckets.data[decoded.bucket_index];
		Asset* asset = &bucket->assets[decoded.elem_index];
		if (asset->handle == handle) {
			return asset;
		}
	}
	return NULL;
}

EXPORT Asset* MakeNewAsset(AssetTree* tree, AssetKind kind) {
	AssetHandleDecoded handle;
	if (tree->first_free_asset.val != ASSET_FREELIST_END) {
		// Take from the freelist
		handle = tree->first_free_asset;
		tree->first_free_asset = tree->asset_buckets[handle.bucket_index]->assets[handle.elem_index].freelist_next;
	}
	else {
		// Allocate a new element
		handle.elem_index = tree->assets_num_allocated % ASSETS_PER_BUCKET;
		if (handle.elem_index == 0) {
			AssetBucket* new_bucket = (AssetBucket*)DS_MemAlloc(DS_HEAP, sizeof(AssetBucket));
			memset(new_bucket, 0, sizeof(AssetBucket));
			DS_ArrPush(&tree->asset_buckets, new_bucket);
		}
		handle.bucket_index = tree->asset_buckets.count - 1;
		tree->assets_num_allocated++;
	}

	Asset* asset = &tree->asset_buckets[handle.bucket_index]->assets[handle.elem_index];
	handle.generation = DecodeAssetHandle(asset->handle).generation + 1; // first valid asset generation is always 1

	*asset = {};
	asset->kind = kind;
	asset->handle = EncodeAssetHandle(handle);
	
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
	ComputeStructLayout(tree, struct_type);

	// Sync all struct assets data to the new type layout
	for (int bucket_i = 0; bucket_i < tree->asset_buckets.count; bucket_i++) {
		for (int elem_i = 0; elem_i < ASSETS_PER_BUCKET; elem_i++) {
			Asset* asset = &tree->asset_buckets[bucket_i]->assets[elem_i];
			if (asset->kind != AssetKind_StructData || asset->struct_data.struct_type != struct_type->handle) continue;
			
			TODO();
		}
	}

	//DS_ForBucketArrayEach(Asset, &tree->assets, IT) {
	//	if (IT.elem->kind != AssetKind_StructData || IT.elem->struct_data.struct_type.asset != struct_type) continue;
	//
	//	TODO();
	//	// we want to also have a DS_Set per struct type that recursively references all struct types it depends on
	//}
}

EXPORT void InitStructDataAsset(AssetTree* tree, Asset* asset, Asset* struct_type) {
	ASSERT(asset->kind == AssetKind_StructData);
	ASSERT(struct_type->kind == AssetKind_StructType);

	asset->struct_data.struct_type = struct_type->handle;
	asset->struct_data.data = DS_MemAlloc(DS_HEAP, struct_type->struct_type.size);
	
	HT_Type type;
	type.kind = HT_TypeKind_Struct;
	type._struct = struct_type->handle;
	Construct(tree, asset->struct_data.data, &type);
	
	// memset(asset->struct_data.data, 0, struct_type->struct_type.size);
	/*for (StructMemberNode* type_node = struct_type->struct_type.root->first_child; type_node; type_node = type_node->next) {
		StructDataMember member;
		memset(&member, 0, sizeof(member));
		DS_ArrPush(&asset->struct_data.members, member);
	}*/
}

EXPORT void DeinitStructDataAssetIfInitialized(AssetTree* tree, Asset* asset) {
	if (asset->struct_data.data) {
		HT_Type type;
		type.kind = HT_TypeKind_Struct;
		type._struct = asset->struct_data.struct_type;
		Destruct(tree, asset->struct_data.data, &type);

		DS_MemFree(DS_HEAP, asset->struct_data.data);
		asset->struct_data.data = NULL;
	}
}

EXPORT void GetTypeSizeAndAlignment(AssetTree* tree, HT_Type* type, i32* out_size, i32* out_alignment) {
	switch (type->kind) {
	case HT_TypeKind_Int:        { *out_size = 4; *out_alignment = 4; } return;
	case HT_TypeKind_Float:      { *out_size = 4; *out_alignment = 4; } return;
	case HT_TypeKind_Bool:       { *out_size = 1; *out_alignment = 1; } return;
	case HT_TypeKind_Vec2:       { *out_size = sizeof(vec2); *out_alignment = alignof(vec2); } return;
	case HT_TypeKind_Vec3:       { *out_size = sizeof(vec3); *out_alignment = alignof(vec3); } return;
	case HT_TypeKind_Vec4:       { *out_size = sizeof(vec4); *out_alignment = alignof(vec4); } return;
	case HT_TypeKind_IVec2:      { *out_size = sizeof(ivec2); *out_alignment = alignof(ivec2); } return;
	case HT_TypeKind_IVec3:      { *out_size = sizeof(ivec3); *out_alignment = alignof(ivec3); } return;
	case HT_TypeKind_IVec4:      { *out_size = sizeof(ivec4); *out_alignment = alignof(ivec4); } return;
	case HT_TypeKind_AssetRef:   { *out_size = sizeof(HT_Asset); *out_alignment = alignof(HT_Asset); } return;
	case HT_TypeKind_Array:      { *out_size = sizeof(HT_Array); *out_alignment = alignof(HT_Array); } return;
	case HT_TypeKind_Any:        { *out_size = sizeof(HT_Any); *out_alignment = alignof(HT_Any); } return;
	case HT_TypeKind_ItemGroup:   { *out_size = sizeof(HT_ItemGroup); *out_alignment = alignof(HT_ItemGroup); } return;
	case HT_TypeKind_String:     { *out_size = sizeof(String); *out_alignment = alignof(String); } return;
	case HT_TypeKind_Struct:     {
		Asset* struct_asset = GetAsset(tree, type->_struct);
		Asset_StructType* struct_type = &struct_asset->struct_type;
		ASSERT(struct_asset != NULL && struct_asset->kind == AssetKind_StructType);

		*out_size = struct_type->size;
		*out_alignment = struct_type->alignment;
	} return;
	case HT_TypeKind_Type:       { *out_size = sizeof(HT_Type); *out_alignment = alignof(HT_Type); } return;
	case HT_TypeKind_COUNT: break;
	case HT_TypeKind_INVALID: break;
	}
}

EXPORT void ComputeStructLayout(AssetTree* tree, Asset* struct_type) {
	ASSERT(struct_type->kind == AssetKind_StructType);
	i32 offset = 0;
	i32 max_alignment = 1;
	
	for (int i = 0; i < struct_type->struct_type.members.count; i++) {
		StructMember* member = &struct_type->struct_type.members[i];
		i32 member_size = 0, member_alignment = 0;
		GetTypeSizeAndAlignment(tree, &member->type, &member_size, &member_alignment);
		
		max_alignment = UI_Max(max_alignment, member_alignment);
		
		offset = DS_AlignUpPow2(offset, member_alignment);
		member->offset = offset;
		offset += member_size;
	}

	struct_type->struct_type.size = DS_AlignUpPow2(offset, max_alignment);
	struct_type->struct_type.alignment = max_alignment;
}

EXPORT void AnyChangeType(AssetTree* tree, HT_Any* any, HT_Type* new_type) {
	if (any->data) AnyDeinit(tree, any);
	
	i32 size, align;
	GetTypeSizeAndAlignment(tree, new_type, &size, &align);
	any->data = DS_MemAlloc(DS_HEAP, size);
	any->type = *new_type;
	Construct(tree, any->data, new_type);
}

EXPORT void AnyDeinit(AssetTree* tree, HT_Any* any) {
	if (any->data) {
		Destruct(tree, any->data, &any->type);
		DS_MemFree(DS_HEAP, any->data);
	}
	DS_DebugFillGarbage(any, sizeof(*any));
}

EXPORT void ArrayPush(HT_Array* array, int32_t elem_size) {
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

EXPORT void ItemGroupInit(HT_ItemGroup* group) {
	*group = {};
	group->first._u32 = HT_ITEM_INDEX_INVALID;
	group->last._u32 = HT_ITEM_INDEX_INVALID;
	group->freelist_first._u32 = HT_ITEM_INDEX_INVALID;
	group->elems_per_bucket = 16;
	group->last_bucket_end = group->elems_per_bucket;
}

EXPORT void ItemGroupDeinit(HT_ItemGroup* group, i32 item_full_size) {
	HT_ItemIndex item_i = group->first;
	while (item_i._u32 != HT_ITEM_INDEX_INVALID) {
		HT_ItemHeader* item = GetItemFromIndex(group, item_i, item_full_size);
		STR_Free(DS_HEAP, {item->name.data, item->name.size});
		item_i = item->next;
	}
	
	for (int i = 0; i < group->buckets.count; i++) {
		void* bucket = ((void**)group->buckets.data)[i];
		DS_MemFree(DS_HEAP, bucket);
	}
	ArrayDeinit(&group->buckets);
	DS_DebugFillGarbage(group, sizeof(*group));
}

EXPORT void MoveItemToAfter(HT_ItemGroup* group, HT_ItemIndex item, HT_ItemIndex move_after_this, i32 item_full_size) {
	HT_ItemIndex prev = move_after_this;
	HT_ItemHeader* prev_p = GetItemFromIndex(group, prev, item_full_size);
	HT_ItemIndex next = prev_p ? prev_p->next : group->first;
	HT_ItemHeader* next_p = GetItemFromIndex(group, next, item_full_size);
	HT_ItemHeader* item_p = GetItemFromIndex(group, item, item_full_size);
	
	item_p->prev = prev;
	item_p->next = next;
	if (prev_p) prev_p->next = item;
	else group->first = item;
	if (next_p) next_p->prev = item;
	else group->last = item;
}

EXPORT HT_ItemIndex ItemGroupAdd(HT_ItemGroup* group, i32 item_full_size) {
	// We could default to a bucket size of say, 16 elements, and provide an option in the UI in the future for tweaking it.
	ASSERT(group->elems_per_bucket > 0); // must be initialized
	
	HT_ItemIndex index;
	HT_ItemHeader* item;
	if (group->freelist_first._u32 != HT_ITEM_INDEX_INVALID) {
		index = group->freelist_first;
		item = GetItemFromIndex(group, index, item_full_size);
		group->freelist_first = item->next;
	}
	else {
		if (group->last_bucket_end == group->elems_per_bucket) {
			// Begin a new bucket
			ArrayPush(&group->buckets, sizeof(void*));
			
			i32 bucket_size = item_full_size * group->elems_per_bucket;
			void* bucket = DS_MemAlloc(DS_HEAP, bucket_size);

			((void**)group->buckets.data)[group->buckets.count - 1] = bucket;
			group->last_bucket_end = 0;
		}
		index.bucket = group->buckets.count - 1;
		index.slot = group->last_bucket_end;
		item = GetItemFromIndex(group, index, item_full_size);
		group->last_bucket_end += 1;
	}
	item->name = {};
	item->prev._u32 = HT_ITEM_INDEX_INVALID;
	item->next._u32 = HT_ITEM_INDEX_INVALID;
	return index;
}

EXPORT HT_ItemHeader* GetItemFromIndex(HT_ItemGroup* group, HT_ItemIndex item, i32 item_full_size) {
	if (item._u32 == HT_ITEM_INDEX_INVALID) return NULL;
	
	ASSERT(item.bucket < group->buckets.count);
	return (HT_ItemHeader*)((char*)((void**)group->buckets.data)[item.bucket] + item_full_size*item.slot);
}

EXPORT void ItemGroupRemove(HT_ItemGroup* group, void* elem, i32 item_client_size) {
	TODO();
}

EXPORT void CalculateItemOffsets(i32 item_size, i32 item_align, i32* out_item_offset, i32* out_item_full_size) {
	*out_item_offset = DS_AlignUpPow2(sizeof(HT_ItemHeader), item_align);
	*out_item_full_size = DS_AlignUpPow2(*out_item_offset + item_size, item_align);
}

EXPORT void StructMemberInit(StructMember* x) {
	StringInit(&x->name);
}
EXPORT void StructMemberDeinit(StructMember* x) {
	StringDeinit(&x->name);
}

EXPORT void StringInit(String* x) { UI_TextInit(DS_HEAP, &x->text, ""); }
EXPORT void StringDeinit(String* x) { UI_TextDeinit(&x->text); }

EXPORT void ArrayClear(HT_Array* array, int32_t elem_size) {
	array->count = 0;
	// TODO: We should shrink the array by default.
}

EXPORT void ArrayDeinit(HT_Array* array) {
	DS_MemFree(DS_HEAP, array->data);
	DS_DebugFillGarbage(array, sizeof(*array));
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
		DeinitStructDataAssetIfInitialized(tree, asset);
	}break;
	}

	asset->kind = AssetKind_FreeSlot;
	
	TODO();
	asset->freelist_next = tree->first_free_asset;
	tree->first_free_asset = DecodeAssetHandle(asset->handle);
}

EXPORT Asset* FindAssetFromPath(AssetTree* tree, Asset* package, STR_View path) {
	Asset* parent = package;
	Asset* result = NULL;

	for (STR_View remaining = path; remaining.size > 0;) {
		STR_View name;
		bool ok = STR_ParseToAndSkip(&remaining, '/', &name);
		ASSERT(ok);

		if (parent == package && STR_CutStart(&name, "$")) {
			u64 name_hash = DS_MurmurHash64A(name.data, name.size, 0);
			Asset* found_package = NULL;
			bool found = DS_MapFind(&tree->package_from_name, name_hash, &found_package);
			if (!found) break;
			
			parent = found_package;
			continue;
		}

		Asset* new_parent = NULL;
		for (Asset* asset = parent->first_child; asset; asset = asset->next) {
			if (STR_MatchCaseInsensitive(UI_TextToStr(asset->name), name)) {
				new_parent = asset;
				break;
			}
		}
		if (new_parent == NULL) break;
		
		parent = new_parent;
		result = parent;
	}

	return result;
}

EXPORT void Construct(AssetTree* tree, void* data, HT_Type* type) {
	i32 size, align;
	GetTypeSizeAndAlignment(tree, type, &size, &align);
	memset(data, 0, size);
	
	if (type->kind == HT_TypeKind_Array) {
	}
	else if (type->kind == HT_TypeKind_ItemGroup) {
		ItemGroupInit((HT_ItemGroup*)data);
	}
	else if (type->kind == HT_TypeKind_Struct) {
		Asset* struct_asset = GetAsset(tree, type->_struct);
		for (int i = 0; i < struct_asset->struct_type.members.count; i++) {
			StructMember* member = &struct_asset->struct_type.members[i];
			Construct(tree, (char*)data + member->offset, &member->type);
		}
	}
}

EXPORT void Destruct(AssetTree* tree, void* data, HT_Type* type) {
	i32 size, align;
	GetTypeSizeAndAlignment(tree, type, &size, &align);
	
	if (type->kind == HT_TypeKind_Array) {
	}
	else if (type->kind == HT_TypeKind_ItemGroup) {
		i32 item_offset, item_full_size;
		CalculateItemOffsets(size, align, &item_offset, &item_full_size);
		ItemGroupDeinit((HT_ItemGroup*)data, item_full_size);
	}
	else if (type->kind == HT_TypeKind_Struct) {
		Asset* struct_asset = GetAsset(tree, type->_struct);
		for (int i = 0; i < struct_asset->struct_type.members.count; i++) {
			StructMember* member = &struct_asset->struct_type.members[i];
			Destruct(tree, (char*)data + member->offset, &member->type);
		}
	}
}
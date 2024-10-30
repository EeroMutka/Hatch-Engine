#define _CRT_SECURE_NO_WARNINGS  // TODO: find a way to get rid of this

#include "test_plugin.hatch_generated.h"

// this should be in the utility libraries...
// #include "../editor_source/fire/fire_ui/fire_ui.c"

#define DS_NO_MALLOC
#include "../editor_source/fire/fire_ds.h"

#include "utility_libraries/hatch_ui.h"

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

#define TODO() __debugbreak()

static void* TempAllocatorProc(struct DS_AllocatorBase* allocator, void* ptr, size_t old_size, size_t size, size_t align) {
	void* data = ((Allocator*)allocator)->ht->TempArenaPush(size, align);
	if (ptr) memcpy(data, ptr, old_size);
	return data;
}

HT_EXPORT void HT_UpdatePlugin(HT_API* ht) {
	// ht->DebugPrint("This is NOT fun!\n");
	
	// hmm... so ideally there would be a duplicate version of fire UI I guess?
	// the question is... how much access do we want to give plugins?
	
	// I think we want to use fire_ui with a special "hatch" backend. We don't want hatch_api to expose a fire_ui state struct.
	// The downside is that the glyph atlas would be duplicated across any plugins that require UI. Not good.
	// So in the hatch API we need to provide access to the fonts atlas and the default font. And we should let
	// you easily allocate and free atlas texture regions.
	
	// Ok, so can we make a fire UI button using a local copy of the fire-UI library?
	// maybe we should first solve the atlas thing yeah...
	
	
	// ok... so first thing first, we need to allocate memory.
	// Then we need fire_ds support.
	
	Allocator _temp_allocator = {{TempAllocatorProc}, ht};
	DS_Allocator* temp_allocator = (DS_Allocator*)&_temp_allocator;
	
	/*DS_Arena test_arena;
	DS_ArenaInit(&test_arena, 1024, allocator);*/

	DS_DynArray(int) my_things = {temp_allocator};
	DS_ArrPush(&my_things, 1);
	DS_ArrPush(&my_things, 2);
	DS_ArrPush(&my_things, 3);
	DS_ArrPush(&my_things, 4);
	
	// DS_ArrClear(&my_things);
	
	UI_DrawRect(ht, {{200, 50}, {230, 600}}, UI_RED);
	
	ht->DrawText("Hello!", {200, 500}, HT_AlignH_Left, 100, UI_BLUE);
}

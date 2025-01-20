

#include <hatch_api.h>

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>
#include <ht_utils/input/input.h>

//#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>
#include "ht_utils/fire/fire_os_sync.h"

#include "../fg.inc.ht"

class FG {
public:
	static void Init(HT_API* ht_api);
	static void ResetTempArena();
	static HT_API* ht;
	static DS_BasicMemConfig mem;
	//static DS_Arena* temp;
	//static DS_Allocator* heap;

private:
	//static DS_AllocatorBase temp_allocator_wrapper;
	//static DS_AllocatorBase heap_allocator_wrapper;
	static DS_Arena temp_arena;
};

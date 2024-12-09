
// TODO: abstract the D3D11 stuff into its own file?
#define HT_INCLUDE_D3D11_API
#define WIN32_LEAN_AND_MEAN
#include <d3d11.h>
#undef far  // wtf windows.h
#undef near // wtf windows.h

#include <hatch_api.h>

#include <ht_utils/math/math_core.h>
#include <ht_utils/math/math_extras.h>
#include <ht_utils/input/input.h>

#define DS_NO_MALLOC
#include <ht_utils/fire/fire_ds.h>

#include "../fg.inc.ht"

struct Allocator {
	DS_AllocatorBase base;
	HT_API* ht;
};

class FG {
public:
	static void Init(HT_API* ht_api);
	static void ResetTempArena();
	static HT_API* ht;
	static DS_Arena* temp;
	static DS_Allocator* heap;

private:
	static Allocator temp_allocator_wrapper;
	static Allocator heap_allocator_wrapper;
	static DS_Arena temp_arena;
};

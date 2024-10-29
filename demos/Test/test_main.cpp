#include "test_plugin.hatch_generated.h"

#include "utility_libraries/hatch_ui.h"

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
	
	
	
	// We always try to allocate from the top.
	
	// Potential growth/shrink strategy for the future: always calculate how many "height units" the texture needs to be
	// and crudely resize to fit that. Any cached glyphs that fall out of that is bye bye.
	
	// So, we can ask the user for "max slot size", "min slot size", "largest slot X count", "largest slot Y count".
	// Additionally, we can ask for "per-level X percentage". By default it's 50, thus each level gets 2x the previous level slot count.
	// If it's 100, then each level gets 4x the previous level slot count.
	
	UI_DrawRect(ht, {{200, 50}, {230, 600}}, UI_RED);
	
	
}

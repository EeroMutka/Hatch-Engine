#include "editor_source/fire/fire_build.h"

#define ArrCount(X) sizeof(X) / sizeof(X[0])

int main() {
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.disable_aslr = true,
	};

	BUILD_Project hatch;
	BUILD_ProjectInit(&hatch, "Hatch", &opts);
	// BUILD_AddIncludeDir(&hatch, "../editor_source/third_party");
	// BUILD_AddIncludeDir(&hatch, ".."); // Repository root
	
	BUILD_AddSourceFile(&hatch, "../editor_source/main.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_data_model.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_ui.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_editor.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_log.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_serialize.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_plugin_compiler.cpp");
	BUILD_AddSourceFile(&hatch, "../editor_source/ht_editor_render.cpp");
	
	BUILD_AddSourceFile(&hatch, "../editor_source/fire/fire_ui/fire_ui.c");
	BUILD_AddSourceFile(&hatch, "../editor_source/fire/fire_ui/fire_ui_color_pickers.c");
	BUILD_AddSourceFile(&hatch, "../editor_source/fire/fire_ui/fire_ui_extras.c");
	BUILD_AddVisualStudioNatvisFile(&hatch, "../editor_source/fire/fire.natstepfilter");
	BUILD_AddVisualStudioNatvisFile(&hatch, "../editor_source/fire/fire.natvis");

	BUILD_AddDefine(&hatch, "_CRT_SECURE_NO_WARNINGS");
	BUILD_AddLinkerInput(&hatch, "User32.lib");
	BUILD_AddLinkerInput(&hatch, "Ole32.lib"); // for OS_FolderPicker and the COM garbage
	BUILD_AddLinkerInput(&hatch, "Shell32.lib"); // for SHFileOperationW in OS_DeleteDirectory
	BUILD_AddLinkerInput(&hatch, "Comdlg32.lib"); // for GetOpenfileNameW in OS_FolderPicker
	
	// for CloseVisualStudioPDBHandle
	BUILD_AddLinkerInput(&hatch, "Rstrtmgr.lib");
	
	// for D3D12
	BUILD_AddLinkerInput(&hatch, "d3d12.lib");
	BUILD_AddLinkerInput(&hatch, "dxgi.lib");
	BUILD_AddLinkerInput(&hatch, "d3dcompiler.lib");
	
	BUILD_AddSourceFile(&hatch, "../editor_source/third_party/md.c");
	
	BUILD_CreateDirectory("editor_build");
	
	BUILD_Project* projects[] = {&hatch};
	if (!BUILD_CreateVisualStudioSolution("editor_build", ".", "Hatch.sln", projects, 1, BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}

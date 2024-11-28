
function SpecifyWarnings()
	flags "FatalWarnings" -- treat all warnings as errors
	buildoptions "/w14062" -- error on unhandled enum members in switch cases
	buildoptions "/w14456" -- error on shadowed locals
	buildoptions "/wd4101" -- allow unused locals
	linkoptions "-IGNORE:4099" -- disable linker warning: "PDB was not found ...; linking object as if no debug info"
end

workspace "Hatch"
	architecture "x64"
	configurations { "Debug", "Release" }
	location "build_projects"

project "Hatch"
	kind "ConsoleApp"
	language "C++"
	targetdir "build"
	
	SpecifyWarnings()
	
	includedirs "plugin_include"
	
	files "editor_source/**"
	files "plugin_include/ht_utils/fire/**"
	removefiles "editor_source/test_main.cpp"
	
	defines "HT_EDITOR_DX11"
	
	-- for CloseVisualStudioPDBHandle
	links "Rstrtmgr.lib"
	
	-- static libs for D3D12
	-- links { "d3d12.lib", "dxgi.lib", "d3dcompiler.lib"}
	
	-- static libs for D3D11
	links { "d3d11", "d3dcompiler.lib"}
	
	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

project "Test"
	kind "ConsoleApp"
	language "C++"
	targetdir "build"
	
	SpecifyWarnings()
	
	includedirs "plugin_include"
	
	files "editor_source/test_main.cpp"
	
	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

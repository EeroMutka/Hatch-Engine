
function specify_warnings()
	flags "FatalWarnings" -- treat all warnings as errors
	buildoptions "/w14062" -- error on unhandled enum members in switch cases
	buildoptions "/w14456" -- error on shadowed locals
	buildoptions "/wd4101" -- allow unused locals
	linkoptions "-IGNORE:4099" -- disable linker warning: "PDB was not found ...; linking object as if no debug info"
end

workspace "hatch"
	architecture "x64"
	configurations { "Debug", "Release" }
	location("." .. _ACTION)

project "hatch-gen"
	kind "ConsoleApp"
	language "C++"
	targetdir ".build"
	
	specify_warnings()
	
	includedirs "."
	
	files "*"
	files "ht_editor_source/**"
	files "ht_utils/**"
	-- files "plugin_include/ht_utils/fire/**"
	
	defines "HT_GEN"
	defines "HT_IMPORT="
	defines "HT_ALL_STATIC_EXPORTS="
	defines "HT_EDITOR_DX11"
	defines { "HATCH_DIR=\"" .. path.getabsolute(".") .. "\"" }
	
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


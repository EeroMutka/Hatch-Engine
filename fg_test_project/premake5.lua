HATCH_DIR = "C:/dev/Hatch"

function specify_warnings()
	flags "FatalWarnings" -- treat all warnings as errors
	buildoptions "/w14062" -- error on unhandled enum members in switch cases
	buildoptions "/w14456" -- error on shadowed locals
	buildoptions "/wd4101" -- allow unused locals
	linkoptions "-IGNORE:4099" -- disable linker warning: "PDB was not found ...; linking object as if no debug info"
end

workspace "fg_test_project"
	architecture "x64"
	configurations { "Debug", "Release" }
	location ("." .. _ACTION)

project "fg_test_project"
	kind "ConsoleApp"
	language "C++"
	targetdir ".build"

	specify_warnings()

	includedirs "%{HATCH_DIR}"

	defines "HT_EDITOR_DX11"
	defines { "HATCH_DIR=\"" .. HATCH_DIR .. "\"" }
	defines "HT_EXPORT=static"

	defines "HT_IMPORT="

	files "%{HATCH_DIR}/*"

	files "%{HATCH_DIR}/ht_editor_source/**"
	files "%{HATCH_DIR}/ht_utils/**"

	files "C:/dev/Hatch/ht_packages/Scene/**"
	files "C:/dev/Hatch/ht_packages/SceneEdit/**"
	files "C:/dev/Hatch/ht_packages/JoltPhysics/**"
	files "C:/dev/Hatch/ht_packages/FGCourse/**"
	links "C:/dev/Hatch/ht_packages/JoltPhysics/Lib/Jolt.lib"

	defines { "HT_ALL_STATIC_EXPORTS="
		..",HT_STATIC_EXPORTS__SceneEdit"
		..",HT_STATIC_EXPORTS__JoltPhysics"
		..",HT_STATIC_EXPORTS__fg"
		}

	links { "d3d11", "d3dcompiler.lib" } 

	staticruntime "On"
	runtime "Release" -- always use the release version of CRT

	filter "configurations:Debug"
		symbols "On"

	filter "configurations:Release"
		optimize "On"

	filter "files:**.hlsl"
		buildaction "None" -- do not use Visual Studio's built-in HLSL compiler


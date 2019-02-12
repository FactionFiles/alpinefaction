project "DashFaction"
	kind "SharedLib"
	language "C++"
	files { "*.h", "*.c", "*.cpp", "*.rc" }
	defines {
		"NOMINMAX",
		"BUILD_DLL",
		"SUBHOOK_STATIC",
	}
	includedirs {
		"../mod_common",
		"../logger/include",
		"../common/include",
		"../vendor",
		"../vendor/zlib",
		"../vendor/zlib/contrib/minizip",
		"../vendor/unrar",
		"../vendor/d3d8",
		"../vendor/subhook",
		"../vendor/xxhash",
	}
	links {
		"psapi",
		"wininet",
		"version",
		"shlwapi",
		"unrar",
		"unzip",
		"zlib",
		"ModCommon",
		"subhook",
		"logger",
		"Common",
		"xxhash",
		"gdi32",
	}
	
	pchheader "stdafx.h"
	pchsource "stdafx.cpp"
	
	-- fix target name in cross-compilation
	targetextension ".dll"
	targetprefix ""

	configuration "vs*"
		linkoptions { "/DEBUG" } -- generate PDB files
	
	configuration "Debug"
		targetdir "../bin/debug"
	
	configuration "Release"
		targetdir "../bin/release"

	configuration { "linux", "Debug" }
		linkoptions "-Wl,-Map,../bin/debug/DashFaction.map"

	configuration { "linux", "Release" }
		linkoptions "-Wl,-Map,../bin/release/DashFaction.map"

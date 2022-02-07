project "Assimp"
	kind "StaticLib"
	cppdialect "C++17"
	language "C++"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .."/")
	objdir ("bin/intermediates/" .. outputdir .."/%{prj.name}")

	files {
		"./**.cpp",
		"./**.c",
		"./**.cc",
		"./**.inl",
		"./**.h",
		"./**.hpp"
	}

	includedirs {
		"./",
		"./include",
		"./code",
		"./contrib/openddlparser/include",
		"./contrib/rapidjson/include",
		"./contrib/irrXML",
		"./contrib/unzip",
		"./contrib/zlib",
		"./contrib"
	}

	defines{
		"UNICODE",
		"_UNICODE",
		"ASSIMP_BUILD_NO_C4D_IMPORTER",
		"MINIZ_USE_UNALIGNED_LOADS_AND_STORES=0",
		"ASSIMP_IMPORTER_GLTF_USE_OPEN3DGC=1",
		"ASSIMP_BUILD_DLL_EXPORT",
		"ASSIMP_BUILD_NO_X_EXPORTER",
		"ASSIMP_BUILD_NO_IFC_IMPORTER",
		"ASSIMP_BUILD_NO_STEP_IMPORTER",
		"ASSIMP_BUILD_NO_ASSJSON_EXPORTER",
		"_SCL_SECURE_NO_WARNINGS",
		"OPENDDLPARSER_BUILD"
	}

	links {

	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

		defines {
			"_CRT_SECURE_NO_WARNINGS",
			"WIN32",
			"_WINDOWS",
			"WIN32_LEAN_AND_MEAN"
		}

	filter "configurations:Debug"
		defines "TERR3D_DEBUG"
		buildoptions "/MTd"
		optimize "on"

	filter "configurations:Release"
		defines "TERR3D_RELEASE"
		buildoptions "/MT"
		optimize "on"	
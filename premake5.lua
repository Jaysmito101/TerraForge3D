workspace "TerraForge3D"
	architecture "x86"

	configurations{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"


project "TerraForge3D"
	location "."
	kind "WindowedApp"
	cppdialect "C++17"
	language "C++"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .."/")
	objdir ("bin/intermediates/" .. outputdir .."/%{prj.name}")

	files {
		"**.h",
		"**.c",
		"**.cpp",
		"**.rc",
		"**.hpp",
		"**.inl"
	}

	includedirs {
		"./include",
		"./include/assimp/include",
		"./src",
		"./include/imgui/backends",
		"./include/imgui",
		"./",
		"./src/Base"
	}

	libdirs {
		"./libs"
	}

	links {
		"glfw3.lib",
		"glfw3_mt.lib",
		"assimp.lib",
		"Urlmon.lib",
		"libssl_static.lib",
		"libcrypto_static",
		"OpenCL.lib",
		"Crypt32",
		"ws2_32",
		"Pathcch",
		"opengl32"
	}

	postbuildcommands  {
		"xcopy \"$(ProjectDir)Binaries\\Data\" \"$(TargetDir)Data\" /e /y /i /r",
		"{COPY} \"$(ProjectDir)Binaries\\assimp-vc140-mt.dll\" \"$(TargetDir)\""
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"

		defines {
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "configurations:Debug"
		defines "TERR3D_DEBUG"
		buildoptions "/MTd"
		symbols "on"

		links{
			"msvcrtd.lib"
		}


	filter "configurations:Release"
		defines "TERR3D_RELEASE"
		buildoptions "/MT"
		optimize "on"	

		links{
			"msvcrtd.lib"
		}


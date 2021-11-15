workspace "TerraForge3D"
	architecture "x86"
	startproject "TerraForge3D"

	configurations{
		"Debug",
		"Release"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "TerraForge3D/vendor/glfw/include"
IncludeDir["Glad"] = "TerraForge3D/vendor/Glad/include"
IncludeDir["glm"] = "TerraForge3D/vendor/glm"
IncludeDir["ImGui"] = "TerraForge3D/vendor/imgui"
IncludeDir["ImNodes"] = "TerraForge3D/vendor/imnodes"
IncludeDir["Zip"] = "TerraForge3D/vendor/zip"
IncludeDir["ImColorTextEdit"] = "TerraForge3D/vendor/text-editor"
IncludeDir["Lua"] = "TerraForge3D/vendor/lua"

include "TerraForge3D/vendor/GLFW"
include "TerraForge3D/vendor/Glad"
include "TerraForge3D/vendor/imgui"
include "TerraForge3D/vendor/imnodes"
include "TerraForge3D/vendor/zip"
include "TerraForge3D/vendor/text-editor"
include "TerraForge3D/vendor/lua"

project "TerraForge3D"
	location "TerraForge3D"
	kind "WindowedApp"
	cppdialect "C++17"
	language "C++"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .."/")
	objdir ("bin/intermediates/" .. outputdir .."/%{prj.name}")

	files {
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/src/**.hpp",
		"%{prj.name}/src/**.inl",
		"%{prj.name}/src/**.rc",
		"%{prj.name}/vendor/glm/**.hpp",
		"%{prj.name}/vendor/glm/**.inl"
	}

	includedirs {
		"./TerraForge3D/vendor/assimp/include",
		"./TerraForge3D/src",
		"./TerraForge3D/src/Base",
		"./TerraForge3D/vendor",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Zip}/src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.Lua}"
	}

	libdirs {
		"./libs"
	}

	links {
		"Urlmon.lib",
		"libssl_static.lib",
		"libcrypto_static",
		"Crypt32",
		"ws2_32",
		"Pathcch",
		"opengl32",
		"GLFW",
		"Glad",
		"Zip",
		"ImGui",
		"ImNodes",
		"ImColorTextEdit",
		"Lua"
	}

	postbuildcommands  {
		"xcopy \"$(SolutionDir)Binaries\\Data\" \"$(TargetDir)Data\\\" /e /r /y",
		"xcopy \"$(SolutionDir)Binaries\\VCRuntime\" \"$(TargetDir)VCRuntime\\\" /e /r /y"
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

		links
		{
			"IrrXMLd.lib",
			"zlibstaticd.lib",
			"assimp-vc142-mtd.lib"
		}

	filter "configurations:Release"
		defines "TERR3D_RELEASE"
		buildoptions "/MT"
		optimize "on"	

		links
		{
			"IrrXML.lib",
			"zlibstatic.lib",
			"assimp-vc142-mt.lib"
		}
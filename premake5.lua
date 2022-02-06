workspace "TerraForge3D"
	architecture "x86_64"
	startproject "TerraForge3D"

	configurations{
		"Debug",
		"Release"
	}

	flags
	{
		"MultiProcessorCompile"
	}

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "TerraForge3D/vendor/glfw/include"
IncludeDir["Glad"] = "TerraForge3D/vendor/Glad/include"
IncludeDir["glm"] = "TerraForge3D/vendor/glm"
IncludeDir["ImGui"] = "TerraForge3D/vendor/imgui"
IncludeDir["ImNodes"] = "TerraForge3D/vendor/imnodes"
IncludeDir["ImPlot"] = "TerraForge3D/vendor/implot"
IncludeDir["ImGuiNodeEditor"] = "TerraForge3D/vendor/imgui-node-editor"
IncludeDir["Zip"] = "TerraForge3D/vendor/zip"
IncludeDir["ImColorTextEdit"] = "TerraForge3D/vendor/text-editor"
IncludeDir["Lua"] = "TerraForge3D/vendor/lua"
IncludeDir["MuParser"] = "TerraForge3D/vendor/muparser/include"

include "TerraForge3D/vendor/GLFW"
include "TerraForge3D/vendor/Glad"
include "TerraForge3D/vendor/imgui"
include "TerraForge3D/vendor/imnodes"
include "TerraForge3D/vendor/implot"
include "TerraForge3D/vendor/imgui-node-editor"
include "TerraForge3D/vendor/zip"
include "TerraForge3D/vendor/text-editor"
include "TerraForge3D/vendor/lua"
include "TerraForge3D/vendor/assimp"
include "TerraForge3D/vendor/muparser"

include "Tools/ModuleMaker"
include "Tools/ModuleSDK"
include "Tools/PyModuleSDK"

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
		"%{IncludeDir.ImPlot}",
		"%{IncludeDir.Lua}",
		"%{IncludeDir.MuParser}",
		"%{IncludeDir.ImGuiNodeEditor}"
	}

	libdirs {
		"./libs/x64"
	}

	links {
		"Urlmon.lib",
		"Crypt32",
		"ws2_32",
		"Pathcch",
		"opengl32",
		"OpenCL",
		"GLFW",
		"Glad",
		"Zip",
		"ImGui",
		"ImNodes",
		"ImPlot",
		"ImGuiNodeEditor",
		"ImColorTextEdit",
		"Lua",
		"MuParser",
		"Assimp"
	}

	postbuildcommands  {
		"xcopy \"$(SolutionDir)Binaries\\Data\" \"$(TargetDir)Data\\\" /e /r /y",
		"xcopy \"$(SolutionDir)Binaries\\VCRuntime\" \"$(TargetDir)VCRuntime\\\" /e /r /y"
	}

	filter "system:windows"
		staticruntime "On"
		systemversion "latest"
		buildoptions{
			"/openmp"
		}

		defines {
			"TERR3D_WIN32",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS"
		}

	filter "configurations:Debug"
		defines "TERR3D_DEBUG"
		buildoptions "/MTd"
		optimize "on"

		links
		{
			"libcryptoMTd.lib",
			"libsslMTd.lib"
		}

	filter "configurations:Release"
		defines "TERR3D_RELEASE"
		buildoptions "/MT"
		optimize "on"	

		links
		{
			"libcryptoMT.lib",
			"libsslMT.lib"
		}
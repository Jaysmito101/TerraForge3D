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
IncludeDir["Glad"] = "TerraForge3D/vendor/glad/include"
IncludeDir["glm"] = "TerraForge3D/vendor/glm"
IncludeDir["ImGui"] = "TerraForge3D/vendor/imgui"
IncludeDir["ImNodes"] = "TerraForge3D/vendor/imnodes"
IncludeDir["ImPlot"] = "TerraForge3D/vendor/implot"
IncludeDir["ImGuiNodeEditor"] = "TerraForge3D/vendor/imgui-node-editor"
IncludeDir["Zip"] = "TerraForge3D/vendor/zip"
IncludeDir["ImColorTextEdit"] = "TerraForge3D/vendor/text-editor"
IncludeDir["Lua"] = "TerraForge3D/vendor/lua"
IncludeDir["MuParser"] = "TerraForge3D/vendor/muparser/include"
IncludeDir["JSON"] = "TerraForge3D/vendor/json"
IncludeDir["FastNoiseLite"] = "TerraForge3D/vendor/FastNoiseLite"

include "TerraForge3D/vendor/glfw"
include "TerraForge3D/vendor/glad"
include "TerraForge3D/vendor/imgui"
include "TerraForge3D/vendor/imnodes"
include "TerraForge3D/vendor/implot"
include "TerraForge3D/vendor/imgui-node-editor"
include "TerraForge3D/vendor/zip"
include "TerraForge3D/vendor/text-editor"
include "TerraForge3D/vendor/lua"
include "TerraForge3D/vendor/muparser"


project "TerraForge3DLib"
	openmp "On"
	location "TerraForge3D"
	kind "StaticLib"
	cppdialect "C++17"
	language "C++"
	staticruntime "On"
    atl "Static"
	vectorextensions "SSE4.1"
	inlining "Auto"

	targetdir ("bin/" .. outputdir .."/")
	objdir ("bin/intermediates/" .. outputdir .."/%{prj.name}")

	files {
		"./TerraForge3D/src/**.h",
		"./TerraForge3D/src/**.cpp",
		"./TerraForge3D/src/**.hpp",
		"./TerraForge3D/src/**.inl",
		"./TerraForge3D/vendor/glm/**.hpp",
		"./TerraForge3D/vendor/glm/**.inl"
	}

	includedirs {
		"./TerraForge3D/include",
		"./TerraForge3D/include/Utils",
		"./TerraForge3D/include/Base",
		"./TerraForge3D/vendor",
		"./TerraForge3D/vendor/openssl",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.Zip}/src",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ImPlot}",
		"%{IncludeDir.Lua}",
		"%{IncludeDir.MuParser}",
		"%{IncludeDir.JSON}",
		"%{IncludeDir.FastNoiseLite}",
		"%{IncludeDir.ImGuiNodeEditor}"
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
		"MuParser"
	}

	excludes {
		"./TerraForge3D/src/Main.cpp",
		"./TerraForge3D/src/Base/EntryPoint.cpp"
	}

	filter "system:linux"
		defines {
			"NULL=0"
		}

		defines {
			"TERR3D_LINUX"
		}

		libdirs {
			"./libs/linux"
		}

		links {
			"dl",
			"pthread",
			"ssl",
			"crypto"
		}
	
	filter "configurations:Debug"
		defines "TERR3D_DEBUG"
		symbols "on"
	
	filter "configurations:Release"
		defines "TERR3D_RELEASE"
	
	filter "system:windows"
		systemversion "latest"

		defines {
			"TERR3D_WIN32",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS"
		}

		libdirs {
			"./libs/win32/x64"
		}

		filter "configurations:Debug"
			links {
				"libcryptoMTd.lib",
				"libsslMTd.lib"
			}
		
		filter "configurations:Release"
			links {
				"libcryptoMT.lib",
				"libsslMT.lib"
			}


project "TerraForge3D"
	openmp "On"
	location "TerraForge3D"
	kind "WindowedApp"
	cppdialect "C++17"
	language "C++"
	staticruntime "on"
    atl "Static"
	vectorextensions "SSE4.1"
	inlining "Auto"


	targetdir ("bin/" .. outputdir .."/")
	objdir ("bin/intermediates/" .. outputdir .."/%{prj.name}")

	files {
		"./TerraForge3D/src/Main.cpp",
		"./TerraForge3D/src/Base/EntryPoint.cpp"
	}

	includedirs {
		"./TerraForge3D/include",
		"./TerraForge3D/vendor",
		"./TerraForge3D/openssl",
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

	links {
		"TerraForge3DLib"
	}		

	filter "system:windows"
		systemversion "latest"
		defines {
			"TERR3D_WIN32",
			"WIN32_LEAN_AND_MEAN",
			"_CRT_SECURE_NO_WARNINGS"
		}
		files {
			"./TerraForge3D/src/TerraForge3D.rc"
		}

		postbuildcommands  {
			"xcopy \"$(SolutionDir)Binaries\\Data\" \"$(TargetDir)Data\\\" /e /r /y",
			"xcopy \"$(SolutionDir)Binaries\\VCRuntime\" \"$(TargetDir)VCRuntime\\\" /e /r /y"
		}

	filter "system:linux"
		defines {
			"NULL=0" 
		}

		defines {
			"TERR3D_LINUX"
		}

		libdirs	{
			"./libs/linux"
		}

		postbuildcommands {
		}

		links {
			"GLFW",
			"GL",
			"OpenCL",
			"ssl",
			"crypto",
			"Glad",
			"Zip",
			"ImGui",
			"ImNodes",
			"ImPlot",
			"ImGuiNodeEditor",
			"ImColorTextEdit",
			"Lua",
			"MuParser",
			"dl",
			"pthread"
		}

	filter "configurations:Debug"
		defines "TERR3D_DEBUG"
		symbols "on"

	filter "configurations:Release"
		defines "TERR3D_RELEASE"
		optimize "Full"	
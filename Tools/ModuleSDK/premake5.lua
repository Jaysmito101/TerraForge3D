project "ModuleSDK"
	kind "SharedLib"
	language "C++"
	staticruntime "on"
	cppdialect "C++17"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

  includedirs {
	"src",
	"include",
	"../../TerraForge3D/vendor/imgui"
  }

	files
	{	
		"src/**.c",
		"src/**.cpp"
	}
	
	links
	{
		"TerraForge3DLib"
	}

	defines
	{
		"TERR3D_WIN32"
	}

	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"

	filter "system:linux"
		pic "On"
		systemversion "latest"
		cppdialect "C++17"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions "/MTd"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		buildoptions "/MT"

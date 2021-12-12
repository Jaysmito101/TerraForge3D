project "ImGuiNodeEditor"
	kind "StaticLib"
	language "C++"
	staticruntime "on"

	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		"*.cpp",
		"*.inl"
	}

	includedirs {
		"../imgui"
	}

	links
	{
		"ImGui"
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
		buildoptions "/MTd"
		symbols "on"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		buildoptions "/MT"

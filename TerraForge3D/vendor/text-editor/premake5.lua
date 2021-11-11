project "ImColorTextEdit"
	kind "StaticLib"
	language "C"
	staticruntime "on"
	
	targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

	includedirs {
		"../imgui"
	}

	files
	{
		"TextEditor.cpp"
	}

	links
	{
		"ImGui"
	}
	
	filter "system:windows"
		systemversion "latest"

	filter "configurations:Debug"
		runtime "Debug"
		symbols "on"
		buildoptions "/MTd"

	filter "configurations:Release"
		runtime "Release"
		optimize "on"
		buildoptions "/MT"
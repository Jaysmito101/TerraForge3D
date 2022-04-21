---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

project "ImGui"
    kind "StaticLib"
    staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"


    files
    {
        "./Source/imgui.cpp",
        "./Source/imgui_demo.cpp",
        "./Source/imgui_draw.cpp",
        "./Source/imgui_tables.cpp",
        "./Source/imgui_widgets.cpp",
        "./Source/backends/imgui_impl_glfw.cpp"
        -- "./Source/backends/imgui_impl_opengl3.cpp",
        -- "./Source/backends/imgui_impl_vulkan.cpp"
    }

    includedirs
    {
        "../GLFW/Include",
        "./Include",
        "./Include/imgui",
        "./Include/imgui/backends"
    }

    links
    {
        "GLFW"
    }

    defines
    {
        
    }
    
    filter "system:linux"
        pic "On"

    filter "system:windows"
        pic "On"

        defines 
	{ 
		"_CRT_SECURE_NO_WARNINGS"
	}

    filter "system:macosx"
        defines 
	{ 
		"_GLFW_COCOA"
	}

    filter "system:*"

    filter "configurations:Debug*"
        symbols "On"
        runtime "Debug"

    filter "configurations:Release*"
        optimize "Full"
        runtime "Release"    
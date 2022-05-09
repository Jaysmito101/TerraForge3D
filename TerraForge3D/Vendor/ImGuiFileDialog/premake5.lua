---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

project "ImGuiFileDialog"
    kind "StaticLib"
    staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"


    files
    {
        "./Source/ImGuiFileDialog.cpp"
    }

    includedirs
    {
        "./Include",
        "../ImGui/Include",
        "../ImGui/Include/imgui"
    }

    links
    {
        "ImGui"
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

    filter "system:*"

    filter "configurations:Debug*"
        symbols "On"
        runtime "Debug"

    filter "configurations:Release*"
        optimize "Full"
        runtime "Release"    
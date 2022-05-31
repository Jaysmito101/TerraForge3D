--------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

project "Glad"
    kind "StaticLib"
    --- language "C"
    --- staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"


    files
    {
        "./Source/glad.c"
    }

    includedirs
    {
        "./Include"
    }

    links
    {
        "opengl32"
    }

    defines
    {
        
    }
    
    filter "system:linux"
        pic "On"

    filter "system:windows"
        defines 
		{ 
			"_GLFW_WIN32",
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
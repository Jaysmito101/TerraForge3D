---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

project "SPDLog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"


    files
    {
        "./Source/**.hpp",
        "./Source/**.h",
        "./Source/**.cpp",
        "./Source/**.c"
    }

    includedirs
    {
        "./Include"
    }

    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

    filter "system:linux"

        links
        {
            "pthread"
        }

    filter "system:*"
    
    filter "configurations:Debug*"
        symbols "On"
        runtime "Debug"

    filter "configurations:Release*"
        optimize "Full"
        runtime "Release"    
---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

-- The main application porject
project "TerraForge3D"

    language "C++"
    cppdialect "C++20"
    staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"

    files
    {
        "./Source/**.cpp",
        "./Source/**.c"
    }

    includedirs
    {
        "../TerraForge3DLib/Include",
        "./Include",
        "../Vendor/SPDLog/Include"
    }

    links
    {
        "TerraForge3DLib"
    }

    filter "system:windows"
        systemversion "latest"

        files
        {
            "./Source/**.rc"
        }

        defines
        {
            "TF3D_WINDOWS",
            "_CRT_SECURE_NO_WARNINGS"
        }

        postbuildcommands  {
			"xcopy \"$(SolutionDir)Binaries\\Data\" \"$(TargetDir)Data\\\" /e /r /y"
		}
    
    filter "system:linux"
        defines
        {
            "TF3D_LINUX"
        }

        links
        {
            "SPDLog"
        }

    filter "system:macosx"
        defines
        {
            "TF3D_MACOSX"
        }

        links
        {
            "SPDLog"
        }
    
    filter "configurations:Debug*"
        defines
        {
            "TF3D_DEBUG"
        }

        symbols "On"
        runtime "Debug"

    filter "configurations:Release*"
        defines
        {
            "TF3D_RELEASE"
        }

        optimize "Full"
        runtime "Release"

    filter "configurations:*VkCompute"
        defines
        {
            "TF3D_VULKAN_COMPUTE"
        }
    
    filter "configurations:*OpenCL"
        defines
        {
            "TF3D_OPENCL"
        }

    -- Build as a WindowedApp on Windows (WinMain Entry) and as a ConsoleApp on Linux and MacOS
    filter "system:windows"
        kind "WindowedApp"
    filter "system:linux"
        kind "ConsoleApp"
    filter "system:macosx"
        kind "ConsoleApp"


    
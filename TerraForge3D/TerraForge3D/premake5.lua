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
        "%{IncludeDirectories.VulkanSDK}",
        "../Vendor/SPDLog/Include",
        "../Vendor/Glad/Include",
        "../Vendor/JSON/Include",
        "../Vendor/STB/Include",
        "../Vendor/IconFontCppHeaders/Include",
        "../Vendor/ImGui/Include"
    }

    libdirs
    {
        "%{LibraryDirectories.VulkanSDK}"
    }

    links
    {
        "TerraForge3DLib"
    }

    defines
    {
        "SPDLOG_COMPILED_LIB"
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
            "dl",
            "uuid",
            "pthread",
            "opengl32",
            "vulkan-1",
            "SPDLog",
            "GLFW",
            "Glad",
            "ImGui"
        }

    filter "system:macosx"
        defines
        {
            "TF3D_MACOSX"
        }

        links
        {
            "SPDLog",
            "GLFW"
        }

    filter "system:*"
    
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


    -- Build as a WindowedApp on Windows (WinMain Entry) and as a ConsoleApp on Linux and MacOS
    filter "system:windows"
        kind "WindowedApp"
    filter "system:linux"
        kind "ConsoleApp"
    filter "system:macosx"
        kind "ConsoleApp"


    
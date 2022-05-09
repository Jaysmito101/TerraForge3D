---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

-- The TerraForge3D Library porject
project "TerraForge3DLib"
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
        "./Include",
        "../Vendor/SPDLog/Include",
        "../Vendor/GLFW/Include",
        "../Vendor/Glad/Include",
        "../Vendor/STB/Include",
        "../Vendor/IconFontCppHeaders/Include",
        "../Vendor/JSON/Include",
        "../Vendor/IconFontCppHeaders/Include",
        "../Vendor/ImGui/Include",
        "../Vendor/ImGuiFileDialog/Include",
        "%{IncludeDirectories.VulkanSDK}"
    }

    libdirs
    {
        "%{LibraryDirectories.VulkanSDK}"
    }

    links
    {
        "vulkan-1",
        "SPDLog",
    	"GLFW",
	"ImGui",
	"ImGuiFileDialog",
	"Glad",
	"opengl32"
    }

    defines
    {
        "SPDLOG_COMPILED_LIB"
    }

    filter "system:windows"
        systemversion "latest"

        defines
        {
            "TF3D_WINDOWS",
            "_CRT_SECURE_NO_WARNINGS"
        }
    
    filter "system:linux"
        defines
        {
            "TF3D_LINUX"
        }

        links
        {
            "uuid"
        }

    filter "system:macosx"
        defines
        {
            "TF3D_MACOSX"
        }

        links
        {
            -- TODO: Add required libs here
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
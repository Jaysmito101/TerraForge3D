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
        "../Vendor/SPDLog/Include"

    }

    links
    {
        "SPDLog"
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
            -- TODO: Add required libs here
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




    
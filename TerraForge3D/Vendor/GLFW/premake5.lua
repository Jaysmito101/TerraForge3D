---------------------------------------------------------------
-- This file belongs to the TerraForge3D project.
--
-- Maintainers:
--   - Jaysmito Mukherjee
--
-- This is under MIT license.
---------------------------------------------------------------

project "GLFW"
    kind "StaticLib"
    --- language "C"
    --- staticruntime "On"

    targetdir "%{wks.location}/bin/%{OutputDir}/%{prj.name}"
    objdir "%{wks.location}/bin/obj/%{OutputDir}/%{prj.name}"


    files
    {
        "./Source/context.c",
        "./Source/init.c",
        "./Source/input.c",
        "./Source/monitor.c",
        "./Source/platform.c",
        "./Source/vulkan.c",
        "./Source/window.c",
        "./Source/egl_context.c",
        "./Source/osmesa_context.c",
        "./Source/null_window.c",
        "./Source/null_joystick.c",
        "./Source/null_monitor.c",
        "./Source/null_init.c"
    }

    includedirs
    {
        "./Include"
    }

    defines
    {
        
    }
    
    filter "system:linux"
        pic "On"

        files
        {
            "./Source/x11_init.c",
            "./Source/x11_monitor.c",
            "./Source/x11_window.c",
            "./Source/xkb_unicode.c",
            "./Source/posix_time.c",
            "./Source/posix_thread.c",
            "./Source/posix_module.c",
            "./Source/posix_poll.c",
            "./Source/glx_context.c",
            "./Source/linux_joystick.c"
        }

        links
        {
            "x11",
            "pthread",
            "dl"
        }

        defines
		{
			"_GLFW_X11"
		}

    filter "system:windows"
        pic "On"

        files
        {
            "./Source/win32_init.c",
            "./Source/win32_monitor.c",
            "./Source/win32_window.c",
            "./Source/win32_joystick.c",
            "./Source/win32_time.c",
            "./Source/win32_thread.c",
            "./Source/win32_module.c",
            "./Source/wgl_context.c"
        }

        defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS"
		}

    filter "system:macosx"
         
        files
        {
            "./Source/cocoa_time.c",
            "./Source/posix_module.c",
            "./Source/posix_thread.c",
            "./Source/cocoa_joystick.m",
            "./Source/cocoa_monitor.m",
            "./Source/cocoa_window.m",
            "./Source/nsgl_context.m"
        }

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
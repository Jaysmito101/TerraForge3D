cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(GLFW C)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REUIRED True)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REUIRED True)

set(SOURCES "")

list(APPEND SOURCES "./Source/context.c")
list(APPEND SOURCES "./Source/init.c")
list(APPEND SOURCES "./Source/input.c")
list(APPEND SOURCES "./Source/monitor.c")
list(APPEND SOURCES "./Source/platform.c")
list(APPEND SOURCES "./Source/vulkan.c")
list(APPEND SOURCES "./Source/window.c")
list(APPEND SOURCES "./Source/egl_context.c")
list(APPEND SOURCES "./Source/osmesa_context.c")
list(APPEND SOURCES "./Source/null_window.c")
list(APPEND SOURCES "./Source/null_joystick.c")
list(APPEND SOURCES "./Source/null_monitor.c")
list(APPEND SOURCES "./Source/null_init.c")

if(CMAKE_C_COMPILER_ID STREQUAL "MSVC")
	# On Windows
	list(APPEND SOURCES "./Source/win32_init.c")
	list(APPEND SOURCES "./Source/win32_monitor.c")
	list(APPEND SOURCES "./Source/win32_window.c")
	list(APPEND SOURCES "./Source/win32_joystick.c")
	list(APPEND SOURCES "./Source/win32_time.c")
	list(APPEND SOURCES "./Source/win32_thread.c")
	list(APPEND SOURCES "./Source/win32_module.c")
	list(APPEND SOURCES "./Source/wgl_context.c")
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_definitions(_GLFW_WIN32)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# On Linux
	list(APPEND SOURCES "./Source/x11_init.c")
	list(APPEND SOURCES "./Source/x11_monitor.c")
	list(APPEND SOURCES "./Source/x11_window.c")
	list(APPEND SOURCES "./Source/xkb_unicode.c")
	list(APPEND SOURCES "./Source/posix_time.c")
	list(APPEND SOURCES "./Source/posix_thread.c")
	list(APPEND SOURCES "./Source/posix_module.c")
	list(APPEND SOURCES "./Source/posix_poll.c")
	list(APPEND SOURCES "./Source/glx_context.c")
	list(APPEND SOURCES "./Source/linux_joystick.c")
	target_link_libraries(GLFW
		x11
		pthread
		dl
	)
	add_compile_definitions(_GLFW_X11)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# On MacOS
	list(APPEND SOURCES "./Source/cocoa_time.c")
	list(APPEND SOURCES "./Source/posix_module.c")
	list(APPEND SOURCES "./Source/posix_thread.c")
	list(APPEND SOURCES "./Source/cocoa_joystick.c")
	list(APPEND SOURCES "./Source/cocoa_monitor.c")
	list(APPEND SOURCES "./Source/cocoa_window.c")
	list(APPEND SOURCES "./Source/nsgl_context.c")
	add_compile_definitions(_GLFW_COCOA)
	else()
	message(FATAL_ERROR "Selected compiler is not supported")
endif()

add_library(GLFW STATIC ${SOURCES})

target_include_directories(GLFW
	PUBLIC ./Include
)

target_link_libraries(GLFW
	opengl32
)
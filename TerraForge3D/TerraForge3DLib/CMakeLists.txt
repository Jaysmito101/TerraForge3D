cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# project(TerraForge3DLib CXX)

# Use FindVulkan module added with CMAKE 3.7
if(NOT CMAKE_VERSION VERSION_LESS 3.7.0)
    message(STATUS "Using module to find Vulkan")
    find_package(Vulkan)
endif()

IF(UNIX AND NOT APPLE)
    set(LINUX TRUE)
ENDIF()

IF(WIN32)
    IF(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan-1 vulkan PATHS ${CMAKE_SOURCE_DIR}/libs/vulkan)

        IF(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        ENDIF()
    ENDIF()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_WIN32_KHR")
ELSEIF(LINUX)
    IF(NOT Vulkan_FOUND)
        find_library(Vulkan_LIBRARY NAMES vulkan HINTS "$ENV{VULKAN_SDK}/lib" "${CMAKE_SOURCE_DIR}/libs/vulkan" REQUIRED)

        IF(Vulkan_LIBRARY)
            set(Vulkan_FOUND ON)
            MESSAGE("Using bundled Vulkan library version")
        ENDIF()
    ENDIF()
ENDIF()

IF(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Could not find Vulkan library!")
ELSE()
    message(STATUS ${Vulkan_LIBRARY})
ENDIF()

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#set(CMAKE_C_STANDARD 17)
#set(CMAKE_C_STANDARD_REUIRED True)

set(SOURCES "")

# list(APPEND SOURCES "./Source/.cpp")

file(GLOB_RECURSE SOURCES "./Source/*.cpp")

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# On Windows
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_definitions(TF3D_WINDOWS)
    add_compile_options(/FS)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
        set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
        set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# On Linux
	target_link_libraries(TerraForge3DLib pthread uuid)
	add_compile_definitions(TF3D_LINUX)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# On MacOS
	target_link_libraries(TerraForge3DLib pthread)
	add_compile_definitions(TF3D_MACOSX)
else()
	message(FATAL_ERROR "Selected compiler is not supported")
endif()

add_library(TerraForge3DLib STATIC ${SOURCES})

target_include_directories(TerraForge3DLib
	PUBLIC ./Include
    PUBLIC "$ENV{VULKAN_SDK}/include"
    PUBLIC ../Vendor/Glad/Include
    PUBLIC ../Vendor/GLFW/Include
    PUBLIC ../Vendor/GLM/Include
    PUBLIC ../Vendor/IconFontCppHeaders/Include
    PUBLIC ../Vendor/ImGui/Include
    PUBLIC ../Vendor/ImGuiFileDialog/Include
    PUBLIC ../Vendor/JSON/Include
    PUBLIC ../Vendor/SPDLog/Include
    PUBLIC ../Vendor/STB/Include
)

target_link_libraries(TerraForge3DLib
    ${Vulkan_LIBRARY}
    Glad
    GLFW
    ImGui
    ImGuiFileDialog
    SPDLog
    opengl32
)
target_link_libraries(TerraForge3DLib debug "$ENV{VULKAN_SDK}/lib/shaderc_sharedd.lib")
target_link_libraries(TerraForge3DLib optimized "$ENV{VULKAN_SDK}/lib/shaderc_shared.lib")

add_compile_definitions(TerraForge3DLib
    Lib_COMPILED_LIB
    SPDLOG_COMPILED_LIB
)

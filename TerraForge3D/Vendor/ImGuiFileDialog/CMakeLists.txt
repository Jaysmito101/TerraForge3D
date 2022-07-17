cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# project(ImGuiFileDialog CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REUIRED True)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REUIRED True)

set(SOURCES "")

list(APPEND SOURCES "./Source/ImGuiFileDialog.cpp")

# add_subdirectory(../ImGui)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# On Windows
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
# On Linux
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
# On MacOS
else()
	message(FATAL_ERROR "Selected compiler is not supported")
endif()

add_library(ImGuiFileDialog STATIC ${SOURCES})

target_include_directories(ImGuiFileDialog
	PUBLIC ../ImGui/Include
	PUBLIC ../ImGui/Include/imgui
	PUBLIC ./Include
)

target_link_libraries(ImGuiFileDialog
	ImGui
)
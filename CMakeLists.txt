cmake_minimum_required(VERSION 3.20)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(TerraForge3D LANGUAGES C CXX)

if(APPLE)
    set(EXECUTABLE_NAME "tf3d")
else()
    set(EXECUTABLE_NAME "terraforge3d")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

add_subdirectory(TerraForge3D/vendor/glfw)

add_library(Glad STATIC TerraForge3D/vendor/glad/src/glad.c)
target_include_directories(Glad PUBLIC TerraForge3D/vendor/glad/include)

add_library(ImGui STATIC
    TerraForge3D/vendor/imgui/imgui.cpp
    TerraForge3D/vendor/imgui/imgui_draw.cpp
    TerraForge3D/vendor/imgui/imgui_tables.cpp
    TerraForge3D/vendor/imgui/imgui_widgets.cpp
    TerraForge3D/vendor/imgui/imgui_demo.cpp
)
target_include_directories(ImGui PUBLIC TerraForge3D/vendor/imgui)

file(GLOB IMPLOT_SOURCES CONFIGURE_DEPENDS TerraForge3D/vendor/implot/*.cpp)
add_library(ImPlot STATIC ${IMPLOT_SOURCES})
target_include_directories(ImPlot PUBLIC TerraForge3D/vendor/implot TerraForge3D/vendor/imgui)
target_link_libraries(ImPlot PUBLIC ImGui)

file(GLOB NODE_EDITOR_SOURCES CONFIGURE_DEPENDS TerraForge3D/vendor/imgui-node-editor/*.cpp)
add_library(ImGuiNodeEditor STATIC ${NODE_EDITOR_SOURCES})
target_include_directories(ImGuiNodeEditor PUBLIC TerraForge3D/vendor/imgui-node-editor TerraForge3D/vendor/imgui)
target_link_libraries(ImGuiNodeEditor PUBLIC ImGui)

add_library(ImColorTextEdit STATIC TerraForge3D/vendor/text-editor/TextEditor.cpp)
target_include_directories(ImColorTextEdit PUBLIC TerraForge3D/vendor/text-editor TerraForge3D/vendor/imgui)
target_link_libraries(ImColorTextEdit PUBLIC ImGui)

file(GLOB MUPARSER_SOURCES CONFIGURE_DEPENDS TerraForge3D/vendor/muparser/src/*.cpp)
add_library(MuParser STATIC ${MUPARSER_SOURCES})
target_compile_definitions(MuParser PUBLIC MUPARSER_STATIC UNICODE _UNICODE)
target_include_directories(MuParser PUBLIC TerraForge3D/vendor/muparser/include)

add_library(Zip STATIC TerraForge3D/vendor/zip/src/zip.c)
target_include_directories(Zip PUBLIC TerraForge3D/vendor/zip/src)

find_library(TERR3D_OPENCL_LIBRARY NAMES OpenCL
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/win32/x64
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/linux
)
if(NOT TERR3D_OPENCL_LIBRARY)
    message(FATAL_ERROR "OpenCL library not found")
endif()

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    TerraForge3D/src/*.cpp
)
if(WIN32)
    list(APPEND SOURCES TerraForge3D/src/TerraForge3D.rc)
endif()

if(WIN32)
    add_executable(${EXECUTABLE_NAME} WIN32 ${SOURCES})
else()
    add_executable(${EXECUTABLE_NAME} ${SOURCES})
endif()
target_include_directories(${EXECUTABLE_NAME} PRIVATE
    TerraForge3D/include
    TerraForge3D/include/Utils
    TerraForge3D/include/Base
    TerraForge3D/vendor
    TerraForge3D/vendor/CL
    TerraForge3D/vendor/FastNoiseLite
    TerraForge3D/vendor/openssl
    TerraForge3D/vendor/avir
    TerraForge3D/vendor/delaunator
    TerraForge3D/vendor/glm
    TerraForge3D/vendor/glfw/include
    TerraForge3D/vendor/glad/include
    TerraForge3D/vendor/httplib
    TerraForge3D/vendor/json
    TerraForge3D/vendor/stb
    TerraForge3D/vendor/zip/src
)

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE
    $<$<BOOL:${WIN32}>:TERR3D_WIN32>
    $<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
    $<$<BOOL:${WIN32}>:WIN32_LEAN_AND_MEAN>
    $<$<BOOL:${WIN32}>:UNICODE>
    $<$<BOOL:${WIN32}>:_UNICODE>
    $<$<BOOL:${UNIX}>:TERR3D_LINUX>
    $<$<CONFIG:Debug>:TERR3D_DEBUG>
    $<$<CONFIG:Release>:TERR3D_RELEASE>
)

target_link_libraries(${EXECUTABLE_NAME} PRIVATE
    glfw
    Glad
    ImGui
    ImPlot
    ImGuiNodeEditor
    ImColorTextEdit
    MuParser
    Zip
    OpenSSL::SSL
    OpenSSL::Crypto
    ${TERR3D_OPENCL_LIBRARY}
    ${OPENGL_LIBRARIES}
)

if(WIN32)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE opengl32)
elseif(UNIX AND NOT APPLE)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE dl pthread)
endif()

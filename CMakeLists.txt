cmake_minimum_required(VERSION 3.12)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(TerraForge3D C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(TF3D_PROFILER_MODE "FULL" CACHE STRING "Profiler instrumentation mode (OFF, CPU, or FULL)")
set_property(CACHE TF3D_PROFILER_MODE PROPERTY STRINGS OFF CPU FULL)
string(TOUPPER "${TF3D_PROFILER_MODE}" TF3D_PROFILER_MODE_UPPER)
if(NOT TF3D_PROFILER_MODE_UPPER STREQUAL "OFF" AND
   NOT TF3D_PROFILER_MODE_UPPER STREQUAL "CPU" AND
   NOT TF3D_PROFILER_MODE_UPPER STREQUAL "FULL")
    message(FATAL_ERROR "TF3D_PROFILER_MODE must be OFF, CPU, or FULL")
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/MCP.cmake)

if(APPLE)
    set(EXECUTABLE_NAME "tf3d")
else()
    set(EXECUTABLE_NAME "terraforge3d")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REUIRED True)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REUIRED True)

find_package( OpenGL REQUIRED )
set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)

set(TF3D_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/include/Base
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/tinyexr
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/avir
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/delaunator/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/spdlog/include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/glm
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/httplib
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/imgui-node-editor
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/json/single_include
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/stb
    ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/libpng
    ${OPENSSL_INCLUDE_DIR}
)

include_directories(${TF3D_INCLUDE_DIRS})

set( GLFW_BUILD_DOCS OFF CACHE BOOL  "GLFW lib only" )
set( GLFW_INSTALL OFF CACHE BOOL  "GLFW lib only" )

add_subdirectory(./TerraForge3D/vendor/glfw)
add_subdirectory(./TerraForge3D/vendor/glad/cmake)
glad_add_library(glad STATIC API gl:core=4.6)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImGui.cmake)
add_subdirectory(./TerraForge3D/vendor/libwebp)
add_subdirectory(./TerraForge3D/vendor/fmt)

if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
endif()

file(GLOB_RECURSE SOURCES
     ./TerraForge3D/src/**.cpp
     ./TerraForge3D/src/TerraForge3D.rc
)

list(REMOVE_DUPLICATES SOURCES)
list(FILTER SOURCES EXCLUDE REGEX "/MCP/.*\\.cpp$")

# list(APPEND SOURCES ./TerraForge3D/vendor/imgui/backends/imgui_impl_glfw.cpp)
# list(APPEND SOURCES ./TerraForge3D/vendor/imgui/backends/imgui_impl_opengl3.cpp)

add_executable(${EXECUTABLE_NAME} ${SOURCES})

set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD 20)
set_property(TARGET ${EXECUTABLE_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
tf3d_configure_mcp(${EXECUTABLE_NAME})

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${EXECUTABLE_NAME} PRIVATE
        -Wno-deprecated-declarations
        -Wno-macro-redefined
        -Wno-switch
        -Wno-switch-enum
    )
endif()

target_compile_definitions(${EXECUTABLE_NAME} PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_definitions(${EXECUTABLE_NAME} PRIVATE SPDLOG_FMT_EXTERNAL)
if(TF3D_PROFILER_MODE_UPPER STREQUAL "OFF")
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE TF3D_PROFILER_ENABLED=0 TF3D_PROFILER_GPU=0)
elseif(TF3D_PROFILER_MODE_UPPER STREQUAL "CPU")
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE TF3D_PROFILER_ENABLED=1 TF3D_PROFILER_GPU=0)
else()
    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE TF3D_PROFILER_ENABLED=1 TF3D_PROFILER_GPU=1)
endif()

target_link_libraries(${EXECUTABLE_NAME} 
    glfw
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
    glad
    ImGui
)

if(WIN32)
    target_compile_definitions(${EXECUTABLE_NAME} 
        PUBLIC _CRT_SECURE_NO_WARNINGS
        PUBLIC TERR3D_WIN32
    )
endif()

if(APPLE)
    target_link_libraries(${EXECUTABLE_NAME} "-framework Security")
endif()

target_link_libraries(${EXECUTABLE_NAME} OpenSSL::SSL)
target_link_libraries(${EXECUTABLE_NAME} OpenSSL::Crypto)
target_link_libraries(${EXECUTABLE_NAME} fmt::fmt)
target_link_libraries(${EXECUTABLE_NAME} webp)

add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DTF3D_DATA_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}/Binaries/Data
        -DTF3D_DATA_DESTINATION=$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Data
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/copy_runtime_data.cmake"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Third Party License"
        "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Third Party License"
)

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
    set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0") 
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT terraforge3d )
    endif()
endif()

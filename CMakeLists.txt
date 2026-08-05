cmake_minimum_required(VERSION 3.12)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(TerraForge3D C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(TF3D_ENABLE_MCP "Enable the optional cpp-mcp integration" OFF)

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

if(TF3D_ENABLE_MCP)
    set(MCP_BUILD_TESTS OFF CACHE BOOL "Build cpp-mcp tests" FORCE)
    set(MCP_SSL OFF CACHE BOOL "Enable cpp-mcp SSL support" FORCE)

    set(TF3D_CPP_MCP_HTTPLIB_NAMESPACE tf3d_cpp_mcp_httplib)

    add_subdirectory(
        ./TerraForge3D/vendor/cpp-mcp
        ${CMAKE_CURRENT_BINARY_DIR}/cpp-mcp
        EXCLUDE_FROM_ALL
    )

    target_include_directories(mcp BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include
    )

    target_compile_definitions(mcp PRIVATE
        httplib=${TF3D_CPP_MCP_HTTPLIB_NAMESPACE}
    )

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(mcp PRIVATE
            -Wno-deprecated-declarations
            -Wno-reorder-ctor
            -Wno-unused-function
            -Wno-unused-private-field
        )
    endif()

    set_property(TARGET mcp PROPERTY INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include"
    )

    get_target_property(_cpp_mcp_configuration_definitions
        mcp INTERFACE_COMPILE_DEFINITIONS
    )
    set_property(TARGET mcp PROPERTY INTERFACE_COMPILE_DEFINITIONS "")

    get_property(_cpp_mcp_example_targets
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/examples"
        PROPERTY BUILDSYSTEM_TARGETS
    )
    foreach(_cpp_mcp_example_target IN LISTS _cpp_mcp_example_targets)
        target_compile_definitions(${_cpp_mcp_example_target} PRIVATE
            ${_cpp_mcp_configuration_definitions}
        )
    endforeach()
endif()

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

if(TF3D_ENABLE_MCP)
    file(GLOB_RECURSE MCP_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/src/MCP/*.cpp
    )

    add_library(tf3d_mcp STATIC ${MCP_SOURCES})
    set_property(TARGET tf3d_mcp PROPERTY CXX_STANDARD 20)
    set_property(TARGET tf3d_mcp PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET tf3d_mcp PROPERTY INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include;${CMAKE_CURRENT_BINARY_DIR}/gladsources/glad/include;${TF3D_INCLUDE_DIRS}"
    )
    target_compile_definitions(tf3d_mcp PRIVATE
        TF3D_ENABLE_MCP=1
        SPDLOG_FMT_EXTERNAL
        httplib=${TF3D_CPP_MCP_HTTPLIB_NAMESPACE}
        ${_cpp_mcp_configuration_definitions}
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(tf3d_mcp PRIVATE -Wno-deprecated-declarations)
    endif()
    target_link_libraries(tf3d_mcp PRIVATE mcp)

    target_compile_definitions(${EXECUTABLE_NAME} PRIVATE TF3D_ENABLE_MCP=1)
endif()

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

if(TF3D_ENABLE_MCP)
    target_link_libraries(${EXECUTABLE_NAME} tf3d_mcp)
endif()

add_custom_command(TARGET ${EXECUTABLE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Binaries/Data"
        "$<TARGET_FILE_DIR:${EXECUTABLE_NAME}>/Data"
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

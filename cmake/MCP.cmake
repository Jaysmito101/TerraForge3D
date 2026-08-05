include_guard(GLOBAL)

option(TF3D_ENABLE_MCP "Enable the optional cpp-mcp integration" OFF)

function(tf3d_configure_mcp executable_name)
    if(NOT TF3D_ENABLE_MCP)
        return()
    endif()

    set(TF3D_CPP_MCP_HTTPLIB_NAMESPACE tf3d_cpp_mcp_httplib)
    set(TF3D_MCP_LOGGER_SHIM
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/include/MCP/Compat/mcp_logger.h")

    set(MCP_BUILD_TESTS OFF CACHE BOOL "Build cpp-mcp tests" FORCE)
    set(MCP_SSL OFF CACHE BOOL "Enable cpp-mcp SSL support" FORCE)

    add_subdirectory(
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp"
        "${CMAKE_CURRENT_BINARY_DIR}/cpp-mcp"
        EXCLUDE_FROM_ALL
    )

    target_include_directories(mcp BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/spdlog/include
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/fmt/include
    )
    target_compile_definitions(mcp PRIVATE
        httplib=${TF3D_CPP_MCP_HTTPLIB_NAMESPACE}
        SPDLOG_FMT_EXTERNAL
    )
    if(MSVC)
        target_compile_options(mcp PRIVATE "/FI${TF3D_MCP_LOGGER_SHIM}")
    else()
        target_compile_options(mcp PRIVATE "-include${TF3D_MCP_LOGGER_SHIM}")
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(mcp PRIVATE
            -Wno-deprecated-declarations
            -Wno-reorder-ctor
            -Wno-unused-function
            -Wno-unused-private-field
        )
    endif()

    set_property(TARGET mcp PROPERTY INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/spdlog/include;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/fmt/include"
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

    file(GLOB_RECURSE MCP_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/src/MCP/*.cpp
    )
    add_library(tf3d_mcp STATIC ${MCP_SOURCES})
    set_property(TARGET tf3d_mcp PROPERTY CXX_STANDARD 20)
    set_property(TARGET tf3d_mcp PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET tf3d_mcp PROPERTY INCLUDE_DIRECTORIES
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/common;${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/cpp-mcp/include;${TF3D_INCLUDE_DIRS}"
    )
    target_include_directories(tf3d_mcp PRIVATE
        "$<TARGET_PROPERTY:glad,INTERFACE_INCLUDE_DIRECTORIES>"
        "$<TARGET_PROPERTY:webp,INTERFACE_INCLUDE_DIRECTORIES>"
    )
    add_dependencies(tf3d_mcp glad webp)
    target_compile_definitions(tf3d_mcp PRIVATE
        TF3D_ENABLE_MCP=1
        SPDLOG_FMT_EXTERNAL
        httplib=${TF3D_CPP_MCP_HTTPLIB_NAMESPACE}
        ${_cpp_mcp_configuration_definitions}
    )
    if(MSVC)
        target_compile_options(tf3d_mcp PRIVATE "/FI${TF3D_MCP_LOGGER_SHIM}")
    else()
        target_compile_options(tf3d_mcp PRIVATE "-include${TF3D_MCP_LOGGER_SHIM}")
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(tf3d_mcp PRIVATE -Wno-deprecated-declarations)
    endif()
    target_link_libraries(tf3d_mcp PRIVATE mcp)

    target_compile_definitions(${executable_name} PRIVATE TF3D_ENABLE_MCP=1)
    target_link_libraries(${executable_name} tf3d_mcp)
endfunction()

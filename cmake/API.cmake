include_guard(GLOBAL)

option(TF3D_ENABLE_API "Enable the optional TerraForge3D REST API" OFF)

function(tf3d_configure_api executable_name)
    if(NOT TF3D_ENABLE_API)
        return()
    endif()

    file(GLOB_RECURSE API_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/src/API/*.cpp"
    )

    add_library(tf3d_api STATIC ${API_SOURCES})
    set_target_properties(tf3d_api PROPERTIES FOLDER "API")
    set_property(TARGET tf3d_api PROPERTY CXX_STANDARD 20)
    set_property(TARGET tf3d_api PROPERTY CXX_STANDARD_REQUIRED ON)
    target_compile_definitions(tf3d_api PRIVATE
        TF3D_ENABLE_API=1
        SPDLOG_FMT_EXTERNAL
    )
    target_include_directories(tf3d_api PRIVATE
        "$<TARGET_PROPERTY:glad,INTERFACE_INCLUDE_DIRECTORIES>"
        "$<TARGET_PROPERTY:webp,INTERFACE_INCLUDE_DIRECTORIES>"
    )
    target_include_directories(tf3d_api PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/TerraForge3D/vendor/httplib"
    )
    add_dependencies(tf3d_api glad webp)

    find_package(Threads REQUIRED)
    target_link_libraries(tf3d_api PUBLIC Threads::Threads)
    if(WIN32)
        target_link_libraries(tf3d_api PUBLIC ws2_32)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(tf3d_api PRIVATE -Wno-deprecated-declarations)
    endif()

    target_compile_definitions(${executable_name} PRIVATE TF3D_ENABLE_API=1)
    target_link_libraries(${executable_name} tf3d_api)
endfunction()

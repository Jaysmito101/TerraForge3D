cmake_minimum_required(VERSION 3.12)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(SPDLog CXX)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REUIRED True)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REUIRED True)

set(SOURCES "")

list(APPEND SOURCES "./Source/async.cpp")
list(APPEND SOURCES "./Source/cfg.cpp")
list(APPEND SOURCES "./Source/color_sinks.cpp")
list(APPEND SOURCES "./Source/file_sinks.cpp")
list(APPEND SOURCES "./Source/fmt.cpp")
list(APPEND SOURCES "./Source/spdlog.cpp")
list(APPEND SOURCES "./Source/stdout_sinks.cpp")

add_compile_definitions(SPDLOG_COMPILED_LIB)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	# On Windows
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	# On Linux
	target_link_libraries(SPDLog pthread)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	# On MacOS
	target_link_libraries(SPDLog pthread)
else()
	message(FATAL_ERROR "Selected compiler is not supported")
endif()

add_library(SPDLog STATIC ${SOURCES})

target_include_directories(SPDLog
	PUBLIC ./Include
)

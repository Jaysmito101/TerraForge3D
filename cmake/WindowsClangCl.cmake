if(NOT WIN32)
    message(FATAL_ERROR "WindowsClangCl.cmake is only supported on Windows.")
endif()

find_program(TERRA_CLANG_CL clang-cl REQUIRED)
find_program(TERRA_LLD_LINK lld-link REQUIRED)
find_program(TERRA_LLVM_LIB llvm-lib REQUIRED)
find_program(TERRA_LLVM_RC llvm-rc REQUIRED)
find_program(TERRA_LLVM_MT llvm-mt REQUIRED)

if(NOT DEFINED CMAKE_C_COMPILER)
    set(CMAKE_C_COMPILER "${TERRA_CLANG_CL}" CACHE FILEPATH "C compiler")
endif()

if(NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "${TERRA_CLANG_CL}" CACHE FILEPATH "CXX compiler")
endif()

if(NOT DEFINED CMAKE_LINKER)
    set(CMAKE_LINKER "${TERRA_LLD_LINK}" CACHE FILEPATH "Linker")
endif()

if(NOT DEFINED CMAKE_AR)
    set(CMAKE_AR "${TERRA_LLVM_LIB}" CACHE FILEPATH "Archiver")
endif()

if(NOT DEFINED CMAKE_RC_COMPILER)
    set(CMAKE_RC_COMPILER "${TERRA_LLVM_RC}" CACHE FILEPATH "Resource compiler")
endif()

if(NOT DEFINED CMAKE_MT)
    set(CMAKE_MT "${TERRA_LLVM_MT}" CACHE FILEPATH "Manifest tool")
endif()

cmake_minimum_required(VERSION 3.2)

project(mozart++)
include_directories(.)

enable_testing()

#Compiler Options

set(CMAKE_MODULE_PATH "${CMCMAKE_MODULE_PATH}" "${CMAKE_SOURCE_DIR}/cmake")

include(CheckIncludeFiles)
include(CheckCXXCompilerFlag)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)

#### Check C++14
if (WIN32)
    set(CMAKE_CXX_STANDARD 14)
else ()
    check_cxx_compiler_flag("-std=c++14" COMPILER_SUPPORTS_CXX14)
    if (COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_STANDARD 14)
    else ()
        message(FATAL "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
    endif ()
endif ()

#### Check C99
if (WIN32)
    set(CMAKE_C_STANDARD 99)
else ()
    check_c_compiler_flag("-std=c99" COMPILER_SUPPORTS_C99)
    if (COMPILER_SUPPORTS_C99)
        set(CMAKE_C_STANDARD 99)
    else ()
        message(FATAL "The compiler ${CMAKE_C_COMPILER} has no C99 support. Please use a different C compiler.")
    endif ()
endif ()

#Source Code

set(SOURCE_CODE
        src/core.cpp
        src/dummy.cpp
        src/process.cpp
        src/process_unix.cpp
        src/process_win32.cpp
        src/timer.cpp)

# Static Library
message("add library mozart++")
add_library(mozart++ STATIC ${SOURCE_CODE})

#test
## test and benchmark targets here
file(GLOB_RECURSE CPP_SRC_LIST tests/test-*.cpp)
foreach(v ${CPP_SRC_LIST})
    string(REGEX MATCH "tests/.*" relative_path ${v})
    string(REGEX REPLACE "tests/" "" target_name ${relative_path})
    string(REGEX REPLACE ".cpp" "" target_name ${target_name})


    add_executable(mpp_${target_name} ${v})
    target_link_libraries(mpp_${target_name} mozart++)
    add_test(mpp_${target_name} mpp_test-${target_name})
endforeach()

cmake_minimum_required(VERSION 2.6)

project(LSEB CXX)

set(CMAKE_VERBOSE_MAKEFILE OFF)

if (CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "Using GNU c++ compiler")
    # setting flags for GNU compiler
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -pthread")
    set(CMAKE_CXX_FLAGS_RELEASE "-O2")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
else (CMAKE_COMPILER_IS_GNUCXX)
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER} is not a supported compiler")
    # exit due to fatal error
endif (CMAKE_COMPILER_IS_GNUCXX)

# Check for c++11 support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    # exit due to fatal error
endif()

add_subdirectory(payload)
add_subdirectory(generator)
add_subdirectory(ru)

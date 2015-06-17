cmake_minimum_required(VERSION 2.6)

project(LSEB CXX)

set(CMAKE_VERBOSE_MAKEFILE OFF)

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")	
  # using GCC
  set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
  
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using Clang or AppleClang
  set(CMAKE_CXX_FLAGS "-Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
  
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
else()
    message(FATAL_ERROR "${CMAKE_CXX_COMPILER_ID} is not a supported compiler")
    # exit due to fatal error
endif()

# Check for c++11 support
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    # exit due to fatal error
endif()

if(TRANSPORT STREQUAL "TCP")

elseif (TRANSPORT STREQUAL "RSOCKETS")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/")
find_package (RDMA REQUIRED)

else()
    message(FATAL_ERROR "The variable TRANSPORT is not properly set.")
    # exit due to fatal error
endif()

# Adding definition of TRANSPORT layer
add_definitions(-D${TRANSPORT})

# searching for boost 1.57 or newer
find_package(Boost 1.57 REQUIRED COMPONENTS system)

add_subdirectory(transport)
add_subdirectory(generator)
add_subdirectory(ru)
add_subdirectory(bu)
add_subdirectory(test)

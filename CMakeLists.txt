cmake_minimum_required(VERSION 2.6)
project(LSEB CXX C)
set(CMAKE_VERBOSE_MAKEFILE OFF)

#Add internal search path for scripts
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/)

#enable or disable usage of hydra
set(ENABLE_HYDRA OFF CACHE BOOL "Enable or disable hydra launcher.")
if (ENABLE_HYDRA)
	add_definitions(-DHAVE_HYDRA)
endif (ENABLE_HYDRA)

message(STATUS "Using ${CMAKE_CXX_COMPILER_ID} compiler")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")	
  # using GCC
  set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra -Wno-unused-local-typedefs -Wno-unused-parameter")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # using Clang or AppleClang
  set(CMAKE_CXX_FLAGS "-pthread -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O2")
  set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
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

# Adding definition of TRANSPORT layer
add_definitions(-D${TRANSPORT})

# searching for boost 1.54 or newer
find_package(Boost 1.53 REQUIRED COMPONENTS system program_options)

add_subdirectory(transport)
add_subdirectory(generator)
add_subdirectory(ru)
add_subdirectory(bu)
add_subdirectory(test)

include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  lseb
  main.cpp
)

target_link_libraries(
  lseb
  ru
  bu
  ${Boost_LIBRARIES}
)

#optional usage of hydra
if (ENABLE_HYDRA)
	add_subdirectory(launcher)
	target_link_libraries(lseb launcher)
endif (ENABLE_HYDRA)

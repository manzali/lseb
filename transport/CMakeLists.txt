if(TRANSPORT STREQUAL "TCP")

include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  transport
  tcp/socket_tcp.cpp
)

target_link_libraries(
  transport
  ${Boost_LIBRARIES}
)

elseif (TRANSPORT STREQUAL "VERBS")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/")
find_package (RDMA REQUIRED)

include_directories(
  ${LSEB_SOURCE_DIR}
  ${RDMA_INCLUDE_DIRS}
)

add_library(
  transport
  verbs/socket_verbs.cpp
)

target_link_libraries(
  transport
  -libverbs
  ${RDMA_LIBRARIES}
)

elseif (TRANSPORT STREQUAL "LIBFABRIC")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/")
find_package (LibFabric REQUIRED)

include_directories(
  ${LSEB_SOURCE_DIR}
  ${LIBFABRIC_INCLUDE_DIRS}
)

add_library(
  transport
  libfabric/socket_libfabric.cpp
)

target_link_libraries(
  transport
  ${LIBFABRIC_LIBRARIES}
)

else()
    message(FATAL_ERROR "The variable TRANSPORT is not properly set.")
    # exit due to fatal error
endif()

MESSAGE(STATUS "Using ${TRANSPORT} transport layer")
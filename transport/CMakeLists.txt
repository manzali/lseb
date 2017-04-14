if (TRANSPORT STREQUAL "VERBS")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/")
find_package (RDMA REQUIRED)

include_directories(
  ${LSEB_SOURCE_DIR}
  ${RDMA_INCLUDE_DIRS}
)

add_library(
  transport
  verbs/socket.cpp
  verbs/acceptor.cpp
  verbs/connector.cpp
)

target_link_libraries(
  transport
  -libverbs
  ${RDMA_LIBRARIES}
)

add_executable(
  show_devices_attr
  verbs/show_devices_attr.cpp
)

target_link_libraries(
  show_devices_attr
  -libverbs
  ${RDMA_LIBRARIES}
)

elseif (TRANSPORT STREQUAL "FI_VERBS" OR TRANSPORT STREQUAL "FI_TCP")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/")
find_package (LibFabric REQUIRED)

add_library(
  transport
  libfabric/socket.cpp
  libfabric/acceptor.cpp
  libfabric/connector.cpp
  libfabric/domain.cpp
  libfabric/shared.cpp)

target_include_directories(
    transport
    PUBLIC
    ${LSEB_SOURCE_DIR}
    ${LIBFABRIC_INCLUDE_DIRS}
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
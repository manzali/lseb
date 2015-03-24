if(TRANSPORT STREQUAL "TCP")

include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  transport
  transport_tcp.cpp
)

elseif (TRANSPORT STREQUAL "RSOCKETS")

include_directories(
  ${LSEB_SOURCE_DIR}
  ${RDMA_INCLUDE_DIRS}
)

add_library(
  transport
  transport_rsockets.cpp
)

target_link_libraries(
  transport
  ${RDMA_LIBRARIES}
)

else()
    message(FATAL_ERROR "The variable TRANSPORT is not properly set.")
    # exit due to fatal error
endif()

MESSAGE(STATUS "Using ${TRANSPORT} transport layer")
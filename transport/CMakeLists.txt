set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/transport/.")

include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  transport
  transport.cpp
)

add_library(
transport_rsockets
transport_rsockets.cpp 
)

find_package (RDMA REQUIRED)

include_directories(${RDMA_INCLUDE_DIRS})

target_link_libraries(
transport_rsockets
${RDMA_LIBRARIES} -lrdmacm -libverbs
)


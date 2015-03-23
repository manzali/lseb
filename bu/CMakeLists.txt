
include_directories(
  ${LSEB_SOURCE_DIR}
)

if(TRANSPORT STREQUAL "TCP")
  MESSAGE(STATUS "Using TCP...")
  ADD_DEFINITIONS(-DTCP)
  add_executable(
    bu
    bu.cpp
    receiver.cpp
  )
  target_link_libraries(
    bu
    transport
  )
elseif(TRANSPORT STREQUAL "RSOCKETS")
  MESSAGE(STATUS "Using rosckets...")
  ADD_DEFINITIONS(-DRSOCKETS)
  add_executable(
    bu
    bu.cpp
    receiver_rsockets.cpp
  )
  target_link_libraries(
    bu
    transport_rsockets
  )
endif()

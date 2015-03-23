
include_directories(
  ${LSEB_SOURCE_DIR}
)

if(TRANSPORT STREQUAL "TCP")
  MESSAGE(STATUS "Using TCP...")
  ADD_DEFINITIONS(-DTCP)
  add_executable(
    ru
    ru.cpp
    controller.cpp
    accumulator.cpp
    sender.cpp
  )
  target_link_libraries(
    ru
    generator
    transport
  )
elseif(TRANSPORT STREQUAL "RSOCKETS")
  MESSAGE(STATUS "Using RSOCKETS...")
  ADD_DEFINITIONS(-DRSOCKETS)
  add_executable(
    ru
    ru.cpp
    controller.cpp
    accumulator.cpp
    sender_rsockets.cpp
  )
  target_link_libraries(
    ru
    generator
    transport_rsockets
  )
endif()

include_directories(
  ${LSEB_SOURCE_DIR}
)

add_executable(
  bu
  bu.cpp
  receiver.cpp
)

target_link_libraries(
  bu
  transport
)
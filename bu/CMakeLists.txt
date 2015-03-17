
include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  receiver
  receiver.cpp
)

add_executable(
  bu
  bu.cpp
)

target_link_libraries(
  bu
  receiver
  transport
)
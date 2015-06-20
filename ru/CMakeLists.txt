
include_directories(
  ${LSEB_SOURCE_DIR}
)

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
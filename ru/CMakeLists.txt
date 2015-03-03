
include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  controller
  controller.cpp
)

add_library(
  sender
  sender.cpp
)

add_executable(
  ru
  ru.cpp
)

target_link_libraries(
  ru
  controller
  sender
  generator
  common
)

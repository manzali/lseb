
include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  generator
  generator.cpp
)

add_library(
  length_generator
  length_generator.cpp
)

target_link_libraries(
  generator
  length_generator
)
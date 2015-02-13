
include_directories(
  ${LSEB_SOURCE_DIR}
)

add_library(
  length_generator
  length_generator.cpp
)

add_executable(
  t_length_generator
  t_length_generator.cpp
)

target_link_libraries(
  t_length_generator
  length_generator
)


include_directories(
  ${LSEB_SOURCE_DIR}
)

add_executable(
  t_length_generator
  t_length_generator.cpp
)

target_link_libraries(
  t_length_generator
  length_generator
)

add_executable(
  t_iniparser
  t_iniparser.cpp
)
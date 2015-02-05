
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  size_generator
  size_generator.cpp
)

add_executable(
  t_size_generator
  t_size_generator.cpp
)

target_link_libraries(
  t_size_generator
  size_generator
)
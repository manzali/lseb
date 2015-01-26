
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  generator
  generator.cpp
)

add_executable(
  t_generator
  t_generator.cpp
)

target_link_libraries(
  t_generator
  generator
)
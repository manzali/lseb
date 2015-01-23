
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  t_generator
  t_generator.cpp
  generator.cpp
)

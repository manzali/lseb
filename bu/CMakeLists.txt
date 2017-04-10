
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  bu
  builder_unit.cpp
)

target_link_libraries(
  bu
  transport
  log
  ${Boost_LIBRARIES}
)

include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  log
  log.cpp
)

target_link_libraries(
  log
  ${Boost_LIBRARIES}
)
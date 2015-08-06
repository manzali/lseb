
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_library(
  ru
  readout_unit.cpp
  controller.cpp
  accumulator.cpp
)

target_link_libraries(
  ru
  generator
  transport
  ${Boost_LIBRARIES}
)
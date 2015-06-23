
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  ru
  ru.cpp
  controller.cpp
  accumulator.cpp
  sender.cpp
)

target_link_libraries(
  ru
  generator
  transport
  ${Boost_LIBRARIES}
)
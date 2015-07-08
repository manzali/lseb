
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  bu
  bu.cpp
  builder.cpp
  receiver.cpp
)

target_link_libraries(
  bu
  transport
  ${Boost_LIBRARIES}
)
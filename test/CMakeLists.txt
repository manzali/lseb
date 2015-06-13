
include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  t_length_generator
  t_length_generator.cpp
)

target_link_libraries(
  t_length_generator
  length_generator
  ${Boost_LIBRARIES}
)

add_executable(
  t_iniparser
  t_iniparser.cpp
)

target_link_libraries(
  t_iniparser
  ${Boost_LIBRARIES}
)

add_executable(
  t_sender
  t_sender.cpp
)

target_link_libraries(
  t_sender
  transport
)

add_executable(
  t_receiver
  t_receiver.cpp
)

target_link_libraries(
  t_receiver
  transport
)
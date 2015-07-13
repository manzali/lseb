
enable_testing()

include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  t_sender
  t_sender.cpp
)

target_link_libraries(
  t_sender
  transport
  ${Boost_LIBRARIES}
)

add_executable(
  t_receiver
  t_receiver.cpp
)

target_link_libraries(
  t_receiver
  transport
  ${Boost_LIBRARIES}
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

add_test(t_length_generator t_length_generator)

add_executable(
  t_log
  t_log.cpp
)

add_test(t_log t_log)

add_executable(
  t_configuration
  t_configuration.cpp
)

target_link_libraries(
  t_configuration
  ${Boost_LIBRARIES}
)

add_test(t_configuration t_configuration ${LSEB_SOURCE_DIR}/test/test.json)

add_custom_target(
  check COMMAND ${CMAKE_CTEST_COMMAND}  --verbose
  DEPENDS t_length_generator t_iniparser t_log t_configuration
)
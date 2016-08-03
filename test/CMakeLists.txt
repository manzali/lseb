
enable_testing()

include_directories(
  ${LSEB_SOURCE_DIR}
  ${Boost_INCLUDE_DIRS}
)

add_executable(
  t_client
  t_client.cpp
)

target_link_libraries(
  t_client
  transport
  ${Boost_LIBRARIES}
)

add_executable(
  t_server
  t_server.cpp
)

target_link_libraries(
  t_server
  transport
  ${Boost_LIBRARIES}
)

add_executable(
  t_client2
  t_client2.cpp
)

target_link_libraries(
  t_client2
  transport
  ${Boost_LIBRARIES}
)

add_executable(
  t_server2
  t_server2.cpp
)

target_link_libraries(
  t_server2
  transport
  ${Boost_LIBRARIES}
)

#add_executable(
#  t_length_generator
#  t_length_generator.cpp
#)

#target_link_libraries(
#  t_length_generator
#  length_generator
#  ${Boost_LIBRARIES}
#)

#add_test(t_length_generator t_length_generator)

#add_executable(
#  t_log
#  t_log.cpp
#)

#add_test(t_log t_log)

#add_executable(
#  t_configuration
#  t_configuration.cpp
#)

#target_link_libraries(
#  t_configuration
#  ${Boost_LIBRARIES}
#)

#add_test(t_configuration t_configuration ${LSEB_SOURCE_DIR}/test/test.json)

#add_custom_target(
#  check COMMAND ${CMAKE_CTEST_COMMAND}  --verbose
#  DEPENDS t_length_generator t_log t_configuration
#)

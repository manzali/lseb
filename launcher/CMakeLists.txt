include_directories(${HYDRA_INCLUDE_DIR})
add_subdirectory(hydra-simple)

add_library(launcher hydra_launcher.cpp)
target_link_libraries(launcher hydra-simple)
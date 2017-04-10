#include <iostream>

#include "log/log.hpp"

int main() {

  async_log::init();

  async_log::add_console(async_log::severity_level::trace);

  async_log::add_file("t_log", 1024, async_log::severity_level::info);

  LOG_DEBUG<< "test " << "with define";
  async_log::log(async_log::severity_level::debug) << "test " << "w/o define";

  LOG_TRACE << "trace";// not printed to file
  LOG_DEBUG << "debug";// not printed to file
  LOG_INFO << "info";
  LOG_WARNING << "warning";
  LOG_ERROR << "error";
  LOG_FATAL << "fatal";

  return EXIT_SUCCESS;
}

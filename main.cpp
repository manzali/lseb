#include <thread>

#include <cassert>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"

#include "transport/endpoints.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc == 3 && "lseb <config_file> <id>");

  std::ifstream f(argv[1]);
  if (!f) {
    std::cerr << argv[1] << ": No such file or directory\n";
    return EXIT_FAILURE;
  }
  Configuration configuration = read_configuration(f);

  Log::init(
    "LSEB",
    Log::FromString(configuration.get<std::string>("LOG_LEVEL")));

  LOG(INFO) << configuration << std::endl;

  int const id = std::stol(argv[2]);
  assert(id >= 0 && "Negative id");
  int endpoints = get_endpoints(configuration.get_child("ENDPOINTS")).size();
  assert(id < endpoints && "Wrong id");

  BuilderUnit bu(configuration, id);
  ReadoutUnit ru(configuration, id);

  std::thread bu_th(bu);
  std::thread ru_th(ru);

  bu_th.join();
  ru_th.join();

  return 0;
}

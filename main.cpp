#include <thread>

#include <boost/lockfree/spsc_queue.hpp>
#include <boost/program_options.hpp>

#include <cassert>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"

#include "transport/endpoints.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  int id;
  std::string str_conf;

  boost::program_options::options_description desc("Options");

  desc.add_options()("help,h", "Print help messages.")(
    "id,i",
    boost::program_options::value<int>(&id)->required(),
    "Process ID.")(
    "configuration,c",
    boost::program_options::value<std::string>(&str_conf)->required(),
    "Configuration JSON file.");

  try {
    boost::program_options::variables_map vm;
    boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv).options(desc).run(),
      vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }
    boost::program_options::notify(vm);
  } catch (const boost::program_options::error& e) {
    std::cerr << e.what() << std::endl << desc << std::endl;
    return EXIT_FAILURE;
  }

  std::ifstream f(str_conf);
  if (!f) {
    std::cerr << str_conf << ": No such file or directory\n";
    return EXIT_FAILURE;
  }

  Configuration configuration = read_configuration(f);

  Log::init(
    "LSEB",
    Log::FromString(configuration.get<std::string>("LOG_LEVEL")));

  LOG(INFO) << configuration << std::endl;

  std::vector<Endpoint> const endpoints = get_endpoints(
    configuration.get_child("ENDPOINTS"));
  if (id < 0 || id >= endpoints.size()) {
    LOG(ERROR) << "Wrong ID: " << id;
    return EXIT_FAILURE;
  }

  int const max_fragment_size = configuration.get<int>(
    "GENERAL.MAX_FRAGMENT_SIZE");
  if (max_fragment_size <= 0) {
    LOG(ERROR) << "Wrong MAX_FRAGMENT_SIZE: " << max_fragment_size;
    return EXIT_FAILURE;
  }

  int const bulk_size = configuration.get<int>("GENERAL.BULKED_EVENTS");
  if (bulk_size <= 0) {
    LOG(ERROR) << "Wrong BULKED_EVENTS: " << bulk_size;
    return EXIT_FAILURE;
  }

  int const tokens = configuration.get<int>("GENERAL.TOKENS");
  if (tokens <= 0) {
    LOG(ERROR) << "Wrong TOKENS: " << tokens;
    return EXIT_FAILURE;
  }

  size_t const multievent_size = max_fragment_size * bulk_size;

  boost::lockfree::spsc_queue<iovec> free_local_data(tokens);
  boost::lockfree::spsc_queue<iovec> ready_local_data(tokens);

  std::unique_ptr<unsigned char[]> const local_data_ptr(
    new unsigned char[multievent_size * tokens]);
  for (int i = 0; i < tokens; ++i) {
    while (!free_local_data.push( {
      local_data_ptr.get() + i * multievent_size,
      0 })) {
      ;
    }
  }

  BuilderUnit bu(configuration, id, free_local_data, ready_local_data);
  std::thread bu_th(bu);
/*
  tcp_barrier(
    id,
    endpoints.size(),
    endpoints[0].hostname(),
    endpoints[0].port());
*/
  ReadoutUnit ru(configuration, id, free_local_data, ready_local_data);
  std::thread ru_th(ru);

  bu_th.join();
  ru_th.join();

  return EXIT_SUCCESS;
}

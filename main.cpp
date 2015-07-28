#include <thread>

#include <boost/lockfree/spsc_queue.hpp>

#include <cassert>

#include "bu/builder_unit.h"
#include "ru/readout_unit.h"

#include "common/log.hpp"
#include "common/configuration.h"
#include "common/tcp_barrier.h"
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

  std::vector<Endpoint> const endpoints = get_endpoints(
    configuration.get_child("ENDPOINTS"));
  assert(id < endpoints.size() && "Wrong id");

  int const max_fragment_size = configuration.get<int>(
    "GENERAL.MAX_FRAGMENT_SIZE");
  assert(max_fragment_size > 0);
  int const bulk_size = configuration.get<int>("GENERAL.BULKED_EVENTS");
  assert(bulk_size > 0);
  int const tokens = configuration.get<int>("GENERAL.TOKENS");
  assert(tokens > 0);
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

  tcp_barrier(
    id,
    endpoints.size(),
    endpoints[0].hostname(),
    endpoints[0].port());

  // This is for safety because the last process started can run ru before the bu has create the socket
  std::this_thread::sleep_for(std::chrono::seconds(1));

  ReadoutUnit ru(configuration, id, free_local_data, ready_local_data);
  std::thread ru_th(ru);

  bu_th.join();
  ru_th.join();

  return 0;
}

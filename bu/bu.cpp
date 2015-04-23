#include <memory>
#include <string>
#include <algorithm>

#include "common/iniparser.hpp"
#include "common/log.h"
#include "common/dataformat.h"
#include "transport/endpoints.h"
#include "transport/transport.h"

#include "bu/receiver.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc == 3 && "bu <config_file> <id>");

  int const bu_id = std::stol(argv[2]);
  assert(bu_id >= 0 && "Negative bu id");

  Parser parser(argv[1]);

  Log::init("BuilderUnit", Log::FromString(parser.top()("BU")["LOG_LEVEL"]));

  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  Endpoints const ru_endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);
  Endpoints const bu_endpoints = get_endpoints(parser.top()("BU")["ENDPOINTS"]);
  size_t const data_size = std::stol(parser.top()("BU")["RECV_BUFFER"]);

  LOG(INFO) << parser << std::endl;

  assert(bu_id < bu_endpoints.size() && "Wrong bu id");

  // Allocate memory

  std::unique_ptr<unsigned char[]> const data_ptr(
    new unsigned char[data_size * ru_endpoints.size()]);

  // Connections

  BuSocket socket = lseb_listen(
    bu_endpoints[bu_id].hostname(),
    bu_endpoints[bu_id].port());

  std::vector<BuConnectionId> connection_ids;

  int endpoint_count = 0;
  std::transform(
    std::begin(ru_endpoints),
    std::end(ru_endpoints),
    std::back_inserter(connection_ids),
    [&](Endpoint const& endpoint) {
      return lseb_accept(
          socket,
          data_ptr.get() + endpoint_count++ * data_size,
          data_size
      );
    });

  Receiver receiver(bu_id, bu_endpoints.size(), bulk_size, connection_ids);

  while (true) {
    receiver.receive();
  }

}


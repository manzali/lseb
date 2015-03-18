#include <memory>
#include <string>
#include <algorithm>

#include "common/frequency_meter.h"
#include "common/iniparser.hpp"
#include "common/log.h"
#include "common/dataformat.h"
#include "transport/endpoints.h"
#include "transport/transport.h"

#include "bu/receiver.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc == 3 && "bu <config_file> <id>");

  Parser parser(argv[1]);
  size_t const bu_id = std::stol(argv[2]);

  Log::init("BuilderUnit", Log::FromString(parser.top()("BU")["LOG_LEVEL"]));

  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  size_t const data_size = std::stol(parser.top()("GENERAL")["DATA_BUFFER"]);
  Endpoints const ru_endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);
  Endpoints const bu_endpoints = get_endpoints(parser.top()("BU")["ENDPOINTS"]);

  LOG(INFO) << parser << std::endl;

  assert(bu_id < bu_endpoints.size() && "Wrong bu id");

  int server_sock = lseb_listen(
      bu_endpoints[bu_id].hostname(),
      bu_endpoints[bu_id].port()
  );

  std::vector<int> connection_ids;
  std::transform(
      std::begin(ru_endpoints),
      std::end(ru_endpoints),
      std::back_inserter(connection_ids),
      [&](Endpoint const& endpoint) {return lseb_accept(server_sock);}
  );

  size_t const metadata_size = connection_ids.size() * bulk_size
      * sizeof(EventMetaData);

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
      new unsigned char[metadata_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
      pointer_cast<EventMetaData>(metadata_ptr.get()),
      pointer_cast<EventMetaData>(metadata_ptr.get() + metadata_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  Receiver receiver(metadata_range, data_range, bulk_size, connection_ids);

  FrequencyMeter bandwith(1.0);

  while (true) {
    size_t read_bytes = receiver.receive(0);
    bandwith.add(read_bytes);

    if (bandwith.check()) {
      LOG(INFO) << "Bandwith: " << bandwith.frequency() / std::giga::num * 8.
                << " Gb/s";
    }
  }

}


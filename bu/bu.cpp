#include <memory>
#include <string>

#include "common/iniparser.hpp"
#include "common/log.h"
#include "common/dataformat.h"
#include "transport/endpoints.h"
#include "transport/transport.h"

#include "bu/receiver.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc <= 2 && "Missing configuration file as parameter!");

  Parser parser(argv[1]);

  Log::init("BuilderUnit", Log::FromString(parser.top()("BU")["LOG_LEVEL"]));

  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  size_t const data_size = std::stol(parser.top()("GENERAL")["DATA_BUFFER"]);
  Endpoints const ru_endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);
  Endpoints const bu_endpoints = get_endpoints(parser.top()("BU")["ENDPOINTS"]);

  LOG(INFO) << parser << std::endl;

  // Retrieve hostname and port for this host!! Now it is the first (hardcoded)

  std::vector<int> connection_ids;
  int server_sock = lseb_listen(bu_endpoints[0].hostname(), bu_endpoints[0].port());
  for (size_t i = 0; i != ru_endpoints.size(); ++i) {
    connection_ids.push_back(lseb_accept(server_sock)); // blocking call
  }

  size_t const metadata_size = ru_endpoints.size() * bulk_size
      * sizeof(EventMetaData);

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
      new unsigned char[metadata_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
      pointer_cast<EventMetaData>(metadata_ptr.get()),
      pointer_cast<EventMetaData>(metadata_ptr.get() + metadata_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  SharedQueue<MetaDataRange> ready_events_queue;
  SharedQueue<MetaDataRange> sent_events_queue;

  Receiver receiver(metadata_range, data_range, ready_events_queue,
                    sent_events_queue, connection_ids);
  std::thread receiver_th(receiver);

  receiver_th.join();

}


#include <memory>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "common/dataformat.h"
#include "common/log.h"
#include "common/utility.h"
#include "common/iniparser.hpp"
#include "common/frequency_meter.h"

#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/sender.h"

#include "transport/endpoints.h"
#include "transport/transport.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc == 3 && "ru <config_file> <id>");

  Parser parser(argv[1]);
  size_t const ru_id = std::stol(argv[2]);

  Log::init("ReadoutUnit", Log::FromString(parser.top()("RU")["LOG_LEVEL"]));

  size_t const generator_frequency = std::stol(
    parser.top()("GENERATOR")["FREQUENCY"]);
  size_t const mean = std::stol(parser.top()("GENERATOR")["MEAN"]);
  size_t const stddev = std::stol(parser.top()("GENERATOR")["STD_DEV"]);
  size_t const data_size = std::stol(parser.top()("GENERAL")["DATA_BUFFER"]);
  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  Endpoints const bu_endpoints = get_endpoints(parser.top()("BU")["ENDPOINTS"]);
  Endpoints const ru_endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);

  LOG(INFO) << parser << std::endl;

  assert(ru_id < ru_endpoints.size() && "Wrong ru id");

  std::vector<int> connection_ids;
  std::transform(
    std::begin(bu_endpoints),
    std::end(bu_endpoints),
    std::back_inserter(connection_ids),
    [](Endpoint const& endpoint) {
      return lseb_connect(endpoint.hostname(), endpoint.port());
    });

  size_t const max_buffered_events = data_size / (sizeof(EventHeader) + mean);
  size_t const metadata_size = max_buffered_events * sizeof(EventMetaData);

  LOG(INFO)
    << "Metadata buffer can contain "
    << max_buffered_events
    << " events";

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
    new unsigned char[metadata_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
    pointer_cast<EventMetaData>(metadata_ptr.get()),
    pointer_cast<EventMetaData>(metadata_ptr.get() + metadata_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  MetaDataBuffer metadata_buffer(
    std::begin(metadata_range),
    std::end(metadata_range));
  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  LengthGenerator payload_size_generator(mean, stddev);
  Generator generator(
    payload_size_generator,
    metadata_buffer,
    data_buffer,
    ru_id);

  Controller controller(generator, metadata_range, generator_frequency);
  Sender sender(metadata_range, data_range, connection_ids, bulk_size);

  FrequencyMeter frequency(1.0);

  while (true) {
    MetaDataRange ready_events = controller.read();
    MultiEvents multievents = sender.add(ready_events);

    if (multievents.size()) {
      size_t sent_bytes = sender.send(multievents);
      frequency.add(multievents.size() * bulk_size);
      controller.release(
        MetaDataRange(
          std::begin(multievents.front().first),
          std::end(multievents.back().first)));
    }

    if (frequency.check()) {
      LOG(INFO)
        << "Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";
    }
  }
}

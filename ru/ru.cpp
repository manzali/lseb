#include <memory>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>

#include "common/dataformat.h"
#include "common/log.hpp"
#include "common/utility.h"
#include "common/iniparser.hpp"
#include "common/frequency_meter.h"

#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/accumulator.h"
#include "ru/sender.h"

#include "transport/transport.h"
#include "transport/endpoints.h"

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
  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  size_t const meta_size = std::stol(parser.top()("RU")["META_BUFFER"]);
  size_t const data_size = std::stol(parser.top()("RU")["DATA_BUFFER"]);
  Endpoints const ru_endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);
  Endpoints const bu_endpoints = get_endpoints(parser.top()("BU")["ENDPOINTS"]);
  std::chrono::milliseconds ms_timeout(
    std::stol(parser.top()("RU")["MS_TIMEOUT"]));

  LOG(INFO) << parser << std::endl;

  assert(ru_id < ru_endpoints.size() && "Wrong ru id");

  assert(
    meta_size % sizeof(EventMetaData) == 0 && "wrong metadata buffer size");

  LOG(INFO) << "Waiting for connections...";
  std::vector<RuConnectionId> connection_ids;
  std::transform(
    std::begin(bu_endpoints),
    std::end(bu_endpoints),
    std::back_inserter(connection_ids),
    [](Endpoint const& endpoint) {
      return lseb_connect(endpoint.hostname(), endpoint.port());
    });
  LOG(INFO) << "Connections established";

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
    new unsigned char[meta_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
    pointer_cast<EventMetaData>(metadata_ptr.get()),
    pointer_cast<EventMetaData>(metadata_ptr.get() + meta_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  MetaDataBuffer metadata_buffer(
    std::begin(metadata_range),
    std::end(metadata_range));
  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  Accumulator accumulator(metadata_range, data_range, bulk_size);
  Sender sender(connection_ids);

  LengthGenerator payload_size_generator(mean, stddev);
  Generator generator(
    payload_size_generator,
    metadata_buffer,
    data_buffer,
    ru_id);
  Controller controller(generator, metadata_range, generator_frequency);

  FrequencyMeter bandwith(1.0);
  FrequencyMeter frequency(1.0);

  while (true) {
    MultiEvents multievents = accumulator.add(controller.read());

    if (multievents.size()) {

      // Create DataIov
      std::vector<DataIov> data_iovs;
      for (auto& multievent : multievents) {
        data_iovs.push_back(create_iovec(multievent.second, data_range));
      }

      size_t sent_bytes = sender.send(data_iovs, ms_timeout);

      bandwith.add(sent_bytes);

      controller.release(
        MetaDataRange(
          std::begin(multievents.front().first),
          std::end(multievents.back().first)));

      frequency.add(multievents.size() * bulk_size);
    }

    if (frequency.check()) {
      LOG(INFO)
        << "Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";
    }
    if (bandwith.check()) {
      LOG(INFO)
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
    }
  }
}

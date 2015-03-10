#include <memory>
#include <thread>
#include <string>
#include <cstdlib>
#include <cassert>

#include "common/dataformat.h"
#include "common/log.h"
#include "common/utility.h"
#include "common/endpoints.h"
#include "common/iniparser.hpp"

#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/sender.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc <= 2 && "Missing configuration file as parameter!");

  Parser parser(argv[1]);

  Log::init("ReadoutUnit", Log::FromString(parser.top()("RU")["LOG_LEVEL"]));

  size_t const generator_frequency = std::stol(
      parser.top()("GENERATOR")["FREQUENCY"]);
  size_t const mean = std::stol(parser.top()("GENERATOR")["MEAN"]);
  size_t const stddev = std::stol(parser.top()("GENERATOR")["STD_DEV"]);
  size_t const data_size = std::stol(parser.top()("GENERAL")["DATA_BUFFER"]);
  size_t const bulk_size = std::stol(parser.top()("GENERAL")["BULKED_EVENTS"]);
  Endpoints const endpoints = get_endpoints(parser.top()("RU")["ENDPOINTS"]);

  LOG(INFO) << parser << std::endl;

  size_t const max_buffered_events = data_size / (sizeof(EventHeader) + mean);
  size_t const metadata_size = max_buffered_events * sizeof(EventMetaData);

  LOG(INFO) << "Metadata buffer can contain " << max_buffered_events
            << " events";

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
      new unsigned char[metadata_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
      pointer_cast<EventMetaData>(metadata_ptr.get()),
      pointer_cast<EventMetaData>(metadata_ptr.get() + metadata_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  MetaDataBuffer metadata_buffer(std::begin(metadata_range),
                                 std::end(metadata_range));
  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  LengthGenerator payload_size_generator(mean, stddev);
  Generator generator(payload_size_generator, metadata_buffer, data_buffer);

  SharedQueue<MetaDataRange> ready_events_queue;
  SharedQueue<MetaDataRange> sent_events_queue;

  Controller controller(generator, metadata_range, ready_events_queue,
                        sent_events_queue);
  std::thread controller_th(controller, generator_frequency);

  Sender sender(metadata_range, data_range, ready_events_queue,
                sent_events_queue, endpoints, bulk_size);
  std::thread sender_th(sender);

  // Waiting

  controller_th.join();
  sender_th.join();

}


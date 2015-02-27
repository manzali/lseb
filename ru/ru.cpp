#include <memory>
#include <thread>

#include <cstdlib>
#include <cassert>

#include "common/dataformat.h"
#include "common/frequency_meter.h"
#include "common/log.h"
#include "common/utility.h"
#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/sender.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  Log::init("ReadoutUnit", Log::DEBUG);

  assert(argc == 2 && "The frequency is required as parameter!");
  size_t const generator_frequency = atoi(argv[1]);

  size_t const mean = 400;
  size_t const stddev = 200;
  size_t const data_size = 32 * 1024 * 16;

  size_t const bulk_size = 40;

  size_t const max_buffered_events = data_size / (sizeof(EventHeader) + mean);
  size_t const metadata_size = max_buffered_events * sizeof(EventMetaData);

  LOG(INFO) << "Metadata buffer can contain " << max_buffered_events
            << " events";

  LOG(INFO) << "Bulked submissions are composed by " << bulk_size << " events";

  LOG(INFO) << "Asked frequency for events generation is "
            << generator_frequency / 1000000. << " MHz";

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
      new unsigned char[metadata_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
      pointer_cast<EventMetaData>(metadata_ptr.get()),
      pointer_cast<EventMetaData>(metadata_ptr.get() + metadata_size));
  MetaDataBuffer metadata_buffer(metadata_range.begin(), metadata_range.end());
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);
  DataBuffer data_buffer(data_range.begin(), data_range.end());

  LengthGenerator payload_size_generator(mean, stddev);
  Generator generator(payload_size_generator, metadata_buffer, data_buffer);

  SharedQueue<MetaDataRange> ready_events_queue;
  SharedQueue<MetaDataRange> sent_events_queue;

  Controller controller(generator, metadata_range, ready_events_queue,
                        sent_events_queue);
  std::thread controller_th(controller, generator_frequency);

  Sender sender(metadata_range, data_range, ready_events_queue,
                sent_events_queue);
  std::thread sender_th(sender, bulk_size);

  // Waiting

  controller_th.join();
  sender_th.join();

}


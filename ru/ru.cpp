#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include <cstdlib>
#include <cassert>

#include "commons/dataformat.h"
#include "commons/pointer_cast.h"
#include "commons/utility.h"

#include "generator/generator.h"
#include "payload/length_generator.h"
#include "ru/controller.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  assert(argc == 2 && "The frequency is required as parameter!");
  size_t const frequency = atoi(argv[1]);

  size_t const mean = 400;
  size_t const stddev = 200;
  size_t const data_size = 32 * 1024 * 1024 * 16;

  size_t const max_buffered_events = data_size / (sizeof(EventHeader) + mean);
  size_t const metadata_size = max_buffered_events * sizeof(EventMetaData);

  std::cout << "Metadata buffer can contain " << max_buffered_events
            << " events.\n";

  std::cout << "Asked frequency:\t" << frequency << " Hz\n";

  // Allocate memory

  std::unique_ptr<char[]> const metadata_buffer(new char[metadata_size]);
  std::unique_ptr<char[]> const data_ptr(new char[data_size]);

  // Define begin and end pointers

  EventMetaData* const begin_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get());
  EventMetaData* const end_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get() + metadata_size);

  char* const begin_data = data_ptr.get();
  char* const end_data = data_ptr.get() + data_size;

  LengthGenerator payload_size_generator(mean, stddev);
  Generator generator(payload_size_generator, begin_metadata, end_metadata,
                      begin_data, end_data);

  EventsQueue ready_events_queue;
  EventsQueue sent_events_queue;

  Controller controller(generator, begin_metadata, end_metadata,
                        ready_events_queue, sent_events_queue);
  std::thread controller_th(controller, frequency);

  size_t tot_generated_events = 0;
  auto start_time = std::chrono::high_resolution_clock::now();

  while (1) {

    EventMetaDataRange metadata_range = ready_events_queue.pop();
    tot_generated_events += circularDistance(metadata_range.begin(),
                                             metadata_range.end(),
                                             max_buffered_events);

    // Releasing the same events generated
    sent_events_queue.push(metadata_range);

    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const diff_us = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time);

    if (diff_us.count() >= 1000000000) {
      double const seconds = diff_us.count() / 1000000000.;
      std::cout << std::fixed << "Real frequency is:\t"
                << tot_generated_events / seconds << " Hz\n";
      tot_generated_events = 0;
      start_time = std::chrono::high_resolution_clock::now();
    }
  }

  controller_th.join();

}


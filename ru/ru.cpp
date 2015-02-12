#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "commons/dataformat.h"
#include "commons/pointer_cast.h"
#include "commons/shared_queue.h"

#include "generator/generator.h"
#include "payload/length_generator.h"
#include "ru/controller.h"

using namespace lseb;

int main() {

  size_t const frequency = 30000000;
  size_t const mean = 400;
  size_t const stddev = 200;
  size_t const data_size = 32 * 1024 * 1024 * 16;

  size_t const mean_buffered_events = data_size / (sizeof(EventHeader) + mean);
  size_t const metadata_size = mean_buffered_events * sizeof(EventMetaData);

  std::cout << "Metadata buffer can contain " << mean_buffered_events
            << " events.\n";

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

  EventsQueuePtr ready_events_queue(new EventsQueue);
  EventsQueuePtr sent_events_queue(new EventsQueue);

  Controller controller(generator, begin_metadata, end_metadata,
                        ready_events_queue, sent_events_queue);

  std::thread controller_th(controller, frequency);

  size_t tot_generated_events = 0;

  auto start_time = std::chrono::high_resolution_clock::now();

  std::cout << "Asked frequency of " << frequency << " Hz\n";

  while (1) {

    EventMetaDataPair metadata_pair;
    ready_events_queue->pop(metadata_pair);

    size_t generated_events = 0;
    ssize_t const diff = std::distance(metadata_pair.first,
                                       metadata_pair.second);
    if (diff >= 0) {
      generated_events = diff;
    } else {
      generated_events = std::distance(metadata_pair.first, end_metadata);
      generated_events += std::distance(begin_metadata, metadata_pair.second);
    }
    tot_generated_events += generated_events;
/*
    std::cout << "Ready\t[" << metadata_pair.first << ", "
              << metadata_pair.second << "] -> " << generated_events
              << " events\n";
*/
    sent_events_queue->push(metadata_pair);
/*
    std::cout << "Sent\t[" << metadata_pair.first << ", "
              << metadata_pair.second << "] -> " << generated_events
              << " events\n";
*/
    auto const end_time = std::chrono::high_resolution_clock::now();
    auto const diff_us = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end_time - start_time);

    if (diff_us.count() >= 1000000000) {
      double const seconds = diff_us.count() / 1000000000.;
      std::cout << "Real frequency is of " << tot_generated_events / seconds
                << " Hz\n";
      tot_generated_events = 0;
      start_time = std::chrono::high_resolution_clock::now();
    }
  }

  controller_th.join();

}


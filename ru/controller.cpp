#include "ru/controller.h"

#include <chrono>
#include <thread>

#include <cmath>

#include "commons/pointer_cast.h"
#include "commons/utility.h"
#include "commons/frequency_meter.h"
#include "commons/log.h"

namespace lseb {

Controller::Controller(Generator const& generator,
                       EventMetaData* begin_metadata,
                       EventMetaData* end_metadata,
                       EventsQueue& ready_events_queue,
                       EventsQueue& sent_events_queue)
    : m_generator(generator),
      m_begin_metadata(begin_metadata),
      m_end_metadata(end_metadata),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue) {
}

void Controller::operator()(size_t generator_frequency) {

  LOG(INFO) << "Asked frequency: " << generator_frequency << " Hz.";

  // Compute time to sleep
  std::chrono::nanoseconds const ns_to_wait(1000000000 / generator_frequency);

  auto const start_time = std::chrono::high_resolution_clock::now();

  size_t total_generated_events = 0;
  EventMetaData* current_metadata = m_begin_metadata;
  static std::ptrdiff_t const metadata_capacity = std::distance(
      m_begin_metadata, m_end_metadata);

  FrequencyMeter frequency_meter(5);

  while (1) {
    std::this_thread::sleep_for(ns_to_wait);
    auto const ns_elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start_time);

    size_t const events_to_generate = ns_elapsed.count() / ns_to_wait.count()
        - total_generated_events;

    size_t const generated_events = m_generator.generateEvents(
        events_to_generate);

    // Send generated events
    if (generated_events) {
      EventMetaData* const previous_metadata = current_metadata;
      current_metadata = circularForward(current_metadata, m_begin_metadata,
                                         metadata_capacity, generated_events);
      m_ready_events_queue.push(
          EventMetaDataRange(previous_metadata, current_metadata));
      total_generated_events += generated_events;

      frequency_meter.add(generated_events);
    }

    if (frequency_meter.check()) {
      LOG(DEBUG) << std::fixed << "Real frequency is: "
                 << frequency_meter.frequency() << " Hz\n";
    }

    // Receive events to release
    while (!m_sent_events_queue.empty()) {
      EventMetaDataRange metadata_range(m_sent_events_queue.pop());
      size_t const events_to_release = circularDistance(metadata_range.begin(),
                                                        metadata_range.end(),
                                                        metadata_capacity);
      m_generator.releaseEvents(events_to_release);
    }
  }

}

}

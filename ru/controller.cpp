#include "ru/controller.h"

#include <chrono>

#include <cmath>

#include "../common/log.h"
#include "../common/utility.h"

namespace lseb {

Controller::Controller(Generator const& generator,
                       MetaDataRange const& metadata_range,
                       SharedQueue<MetaDataRange>& ready_events_queue,
                       SharedQueue<MetaDataRange>& sent_events_queue)
    : m_generator(generator),
      m_metadata_range(metadata_range),
      m_ready_events_queue(ready_events_queue),
      m_sent_events_queue(sent_events_queue) {
}

void Controller::operator()(size_t generator_frequency) {

  // Compute time to sleep
  std::chrono::nanoseconds const ns_to_wait(1000000000 / generator_frequency);

  auto const start_time = std::chrono::high_resolution_clock::now();

  size_t total_generated_events = 0;
  EventMetaData* current_metadata = m_metadata_range.begin();

  while (true) {
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
      current_metadata = advance_in_range(current_metadata, generated_events,
                                          m_metadata_range);
      m_ready_events_queue.push(
          MetaDataRange(previous_metadata, current_metadata));
      total_generated_events += generated_events;

    }

    // Receive events to release
    while (!m_sent_events_queue.empty()) {
      MetaDataRange metadata_subrange(m_sent_events_queue.pop());
      size_t const events_to_release = distance_in_range(metadata_subrange,
                                                         m_metadata_range);
      m_generator.releaseEvents(events_to_release);
    }
  }

}

}

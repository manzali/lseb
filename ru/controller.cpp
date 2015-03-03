#include "ru/controller.h"

#include <chrono>

#include <cmath>

#include "common/log.h"
#include "common/utility.h"

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

  auto const start_time = std::chrono::high_resolution_clock::now();
  size_t tot_generated_events = 0;
  auto current_metadata = m_metadata_range.begin();
  bool wait_for_events = false;

  while (true) {
    double const elapsed_seconds = std::chrono::duration<double>(
        std::chrono::high_resolution_clock::now() - start_time).count();

    size_t const tot_events_to_generate = elapsed_seconds * generator_frequency;

    assert(tot_events_to_generate >= tot_generated_events);

    // If there are events to generate
    if (tot_events_to_generate != tot_generated_events) {

      size_t const generated_events = m_generator.generateEvents(
          tot_events_to_generate - tot_generated_events);

      // Send generated events
      if (generated_events) {
        auto previous_metadata = current_metadata;
        current_metadata = advance_in_range(current_metadata, generated_events,
                                            m_metadata_range);
        m_ready_events_queue.push(
            MetaDataRange(previous_metadata, current_metadata));
        tot_generated_events += generated_events;
      } else {
        LOG(WARNING) << "Not enough space for events generation!";
        wait_for_events = true;
      }
    }

    // Receive events to release
    while (wait_for_events || !m_sent_events_queue.empty()) {
      size_t const events_to_release = distance_in_range(
          m_sent_events_queue.pop(), m_metadata_range);
      m_generator.releaseEvents(events_to_release);
      wait_for_events = false;
    }
  }

}

}

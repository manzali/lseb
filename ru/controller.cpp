#include "ru/controller.h"

#include <chrono>
#include <thread>

#include <cmath>

#include "commons/pointer_cast.h"

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

void Controller::operator()(size_t frequency) {

  // Compute time to sleep
  std::chrono::nanoseconds const ns_to_wait(1000000000 / frequency);

  auto const start_time = std::chrono::high_resolution_clock::now();

  size_t total_generated_events = 0;

  EventMetaData* current_metadata = m_begin_metadata;

  while (1) {
    std::this_thread::sleep_for(ns_to_wait);
    auto const ns_elapsed =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start_time);

    size_t const events_to_generate = ns_elapsed.count() / ns_to_wait.count()
        - total_generated_events;

    size_t const generated_events = m_generator.generateEvents(
        events_to_generate);

    EventMetaDataPair metadata_pair;
    metadata_pair.first = current_metadata;
    size_t const dist_to_end_metadata = std::distance(current_metadata,
                                                      m_end_metadata);
    current_metadata =
        (generated_events < dist_to_end_metadata) ?
            current_metadata + generated_events :
            m_begin_metadata + generated_events - dist_to_end_metadata;
    metadata_pair.second = current_metadata;

    if (generated_events) {
      m_ready_events_queue.push(metadata_pair);
      total_generated_events += generated_events;
      // Reset metadata_pair
      metadata_pair.second = metadata_pair.first;
      m_sent_events_queue.pop_nowait(metadata_pair);
    } else {
      m_sent_events_queue.pop(metadata_pair);
    }

    size_t events_to_release = 0;

    ssize_t const diff = std::distance(metadata_pair.first,
                                       metadata_pair.second);
    if (diff >= 0) {
      events_to_release = diff;
    } else {
      events_to_release = std::distance(metadata_pair.first, m_end_metadata);
      events_to_release += std::distance(m_begin_metadata, metadata_pair.second);
    }

    m_generator.releaseEvents(events_to_release);

  }

}

}

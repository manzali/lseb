#include "ru/controller.h"

#include "common/utility.h"

namespace lseb {

Controller::Controller(Generator const& generator,
                       MetaDataRange const& metadata_range,
                       size_t generator_frequency)
    : m_generator(generator),
      m_metadata_range(metadata_range),
      m_current_metadata(std::begin(m_metadata_range)),
      m_start_time(std::chrono::high_resolution_clock::now()),
      m_generator_frequency(generator_frequency),
      m_generated_events(0) {
}

MetaDataRange Controller::generate() {

  double const elapsed_seconds = std::chrono::duration<double>(
      std::chrono::high_resolution_clock::now() - m_start_time).count();

  size_t const events_to_generate = elapsed_seconds * m_generator_frequency;
  assert(events_to_generate >= m_generated_events);

  size_t const current_generated_events = m_generator.generateEvents(
      events_to_generate - m_generated_events);
  auto previous_metadata = m_current_metadata;
  m_current_metadata = advance_in_range(m_current_metadata,
                                        current_generated_events,
                                        m_metadata_range);
  m_generated_events += current_generated_events;
  return MetaDataRange(previous_metadata, m_current_metadata);
}

void Controller::release(MetaDataRange metadata_range) {
  size_t const events_to_release = distance_in_range(metadata_range,
                                                     m_metadata_range);
  m_generator.releaseEvents(events_to_release);
}

}

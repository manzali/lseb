#include <iostream>
#include <memory>
#include <vector>

#include <boost/detail/lightweight_test.hpp>

#include "commons/dataformat.h"
#include "commons/pointer_cast.h"
#include "payload/length_generator.h"
#include "generator/generator.h"

using namespace lseb;

int main() {

  size_t const data_size = 32 * 1024;
  std::unique_ptr<char[]> const data_buffer(new char[data_size]);

  size_t const mean = 400;
  size_t const stddev = 200;

  size_t const mean_buffered_events = data_size / (sizeof(EventHeader) + mean);

  size_t metadata_size = mean_buffered_events * sizeof(EventMetaData);
  std::unique_ptr<char[]> const metadata_buffer(new char[metadata_size]);

  EventMetaData* const begin_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get());
  EventMetaData* const end_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get() + metadata_size);

  char* const begin_data = data_buffer.get();
  char* const end_data = data_buffer.get() + data_size;

  LengthGenerator payload_length_generator(mean, stddev);
  Generator generator(payload_length_generator, begin_metadata, end_metadata,
                      begin_data, end_data);

  uint64_t partial_generated_events = 0;
  uint64_t generated_events = 0;
  do {
    partial_generated_events = generator.generateEvents(
        mean_buffered_events / 10);
    generated_events += partial_generated_events;
  } while (partial_generated_events);

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  char const* current_data = data_buffer.get();

  uint64_t current_event_id = 0;

  BOOST_TEST(generated_events > 0);

  EventMetaData const* current_metadata = begin_metadata;

  while (current_event_id < generated_events) {
    // Check MetaData
    BOOST_TEST_EQ(current_metadata->id, current_event_id);
    BOOST_TEST_EQ(current_metadata->offset, current_data - begin_data);
    // Check Data
    BOOST_TEST(current_data + current_metadata->length <= end_data);
    EventHeader const& header = *pointer_cast<EventHeader const>(current_data);
    BOOST_TEST_EQ(header.length, current_metadata->length);
    BOOST_TEST_EQ(header.flags, 0);
    BOOST_TEST_EQ(header.id, current_event_id);
    // missing check on payload

    // Update pointers and increment counters
    ++current_metadata;
    if (current_metadata == end_metadata) {
      current_metadata = begin_metadata;
    }
    current_data += header.length;
    ++current_event_id;
  }

  generator.releaseEvents(mean_buffered_events / 2);

  generated_events = 0;
  do {
    partial_generated_events = generator.generateEvents(100);
    generated_events += partial_generated_events;
  } while (partial_generated_events);

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  BOOST_TEST(generated_events > 0);

  while (current_event_id < generated_events) {
    // Check MetaData
    BOOST_TEST_EQ(current_metadata->id, current_event_id);
    BOOST_TEST_EQ(current_metadata->offset, current_data - begin_data);
    // Check Data
    BOOST_TEST(current_data + current_metadata->length <= end_data);
    EventHeader const& header = *pointer_cast<EventHeader const>(current_data);
    BOOST_TEST_EQ(header.length, current_metadata->length);
    BOOST_TEST_EQ(header.flags, 0);
    BOOST_TEST_EQ(header.id, current_event_id);
    // missing check on payload

    // Update pointers and increment counters
    ++current_metadata;
    if (current_metadata == end_metadata) {
      current_metadata = begin_metadata;
    }
    current_data += header.length;
    ++current_event_id;
  }

  std::cout << "Max event id: " << SIZE_MAX << std::endl;

  boost::report_errors();
}


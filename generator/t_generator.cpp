#include <iostream>
#include <memory>
#include <vector>

#include <boost/detail/lightweight_test.hpp>

#include "commons/dataformat.h"
#include "commons/pointer_cast.h"
#include "payload/size_generator.h"
#include "generator/generator.h"

using namespace lseb;

int main() {

  size_t const data_size = 1024 * 1024;
  std::unique_ptr<char[]> const data_buffer(new char[data_size]);

  size_t const mean = 400;
  size_t const stddev = 200;

  size_t const mean_buffered_events = data_size / (sizeof(EventHeader) + mean);

  size_t metadata_size = mean_buffered_events * sizeof(EventMetaData);
  std::unique_ptr<char[]> const metadata_buffer(new char[metadata_size]);

  SizeGenerator payload_size_generator(mean, stddev);
  Generator generator(data_buffer.get(), payload_size_generator);

  EventMetaData* const begin_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get());
  EventMetaData* const end_metadata = pointer_cast<EventMetaData>(
      metadata_buffer.get() + metadata_size);

  uint64_t const generated_events = generator.generateEvents(
      begin_metadata, end_metadata, data_buffer.get(),
      data_buffer.get() + data_size);

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  char const* const begin_data = data_buffer.get();
  char const* const end_data = data_buffer.get() + data_size;

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
    current_data += header.length;
    ++current_event_id;
  }

  boost::report_errors();
}


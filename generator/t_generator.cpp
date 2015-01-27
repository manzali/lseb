#include <iostream>
#include <memory>

#include <cstdlib>

#include <boost/detail/lightweight_test.hpp>

#include "commons/dataformat.h"
#include "commons/pointer_cast.h"
#include "generator/generator.h"

using namespace lseb;

static size_t const megabyte = 1048576;

int main() {
  size_t const metadata_size = megabyte;
  std::unique_ptr<char[]> const metadata_buffer(new char[metadata_size]);

  size_t const data_size = megabyte;
  std::unique_ptr<char[]> const data_buffer(new char[data_size]);

  Generator generator(400, 200);

  int64_t const generated_events = generator.generateEvents(
      metadata_buffer.get(), metadata_buffer.get() + metadata_size,
      data_buffer.get(), data_buffer.get() + data_size);

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  char const* const begin_data = data_buffer.get();
  char const* const end_data = data_buffer.get() + data_size;

  char const* current_data = data_buffer.get();

  uint64_t current_event_id = 0;

  BOOST_TEST(generated_events > 0);

  EventMetaData const* metadata = pointer_cast<EventMetaData const>(
      metadata_buffer.get());

  while (current_event_id != generated_events) {
    // Check MetaData
    BOOST_TEST_EQ(metadata->id, current_event_id);
    BOOST_TEST_EQ(metadata->offset, current_data - begin_data);
    // Check Data
    BOOST_TEST(current_data + metadata->length <= end_data);
    EventHeader const& header = *pointer_cast<EventHeader const>(current_data);
    BOOST_TEST_EQ(header.length, metadata->length);
    BOOST_TEST_EQ(header.flags, 0);
    BOOST_TEST_EQ(header.id, current_event_id);
    // missing check on payload

    // Update pointers and increment counters
    ++metadata;
    current_data += header.length;
    ++current_event_id;
  }

  boost::report_errors();
}


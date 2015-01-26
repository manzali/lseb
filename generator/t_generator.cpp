#include <iostream>
#include <memory>
#include <utility>

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

  std::pair<char*, char*> current_pointers = generator.generateEvents(
      metadata_buffer.get(), metadata_buffer.get() + metadata_size,
      data_buffer.get(), data_buffer.get() + data_size);

  // Now check consistency between metadata and data

  char const* const begin_data = data_buffer.get();

  char const* current_metadata = metadata_buffer.get();
  char const* current_data = data_buffer.get();

  char const* const end_metadata = current_pointers.first;
  char const* const end_data = current_pointers.second;

  uint64_t current_event_id = 0;

  while (current_metadata != end_metadata) {
    // Check MetaData
    EventMetaData const& metadata = *pointer_cast<EventMetaData const>(
        current_metadata);
    BOOST_TEST_EQ(metadata.id, current_event_id);
    // metadata.length implicitly tested with header.length
    BOOST_TEST_EQ(metadata.offset, current_data - begin_data);

    // Check Data
    BOOST_TEST(current_data + metadata.length <= end_data);
    EventHeader const& header = *pointer_cast<EventHeader const>(current_data);
    BOOST_TEST_EQ(header.length, metadata.length);
    BOOST_TEST_EQ(header.flags, 0);
    BOOST_TEST_EQ(header.id, current_event_id);
    // missing check on payload

    // Update pointers and increment counters
    current_metadata += sizeof(metadata);
    current_data += header.length;
    ++current_event_id;
  }

  std::cout << "Generated " << current_event_id << " events." << std::endl;

  boost::report_errors();
}


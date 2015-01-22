#include <iostream>
#include <memory>

#include <cstdlib>

#include <boost/detail/lightweight_test.hpp>

#include "commons/dataformat.h"
#include "generator/Generator.h"

using namespace lseb;

static size_t const megabyte = 1048576;

int main() {
  size_t const metadata_size = megabyte;
  std::unique_ptr<char[]> const metadata_ptr(new char[metadata_size]);

  size_t const data_size = megabyte;
  std::unique_ptr<char[]> const data_ptr(new char[data_size]);

  Generator generator(400, 200);

  int generated_events = generator.generateEvents(metadata_ptr.get(),
                                                  metadata_size, data_ptr.get(),
                                                  data_size);

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  size_t m_offset = 0;
  size_t d_offset = 0;

  uint64_t current_event_id = 0;

  while (generated_events != 0) {
    // Check MetaData
    EventMetaData const& metadata = *eventmetadata_cast(
        metadata_ptr.get() + m_offset);
    BOOST_TEST_EQ(metadata.id, current_event_id);
    // metadata.length implicitly tested with header.length
    BOOST_TEST_EQ(metadata.offset, d_offset);

    // Check Data
    EventHeader const& header = *eventheader_cast(data_ptr.get() + d_offset);
    BOOST_TEST_EQ(header.length, metadata.length);
    BOOST_TEST_EQ(header.flags, 0);
    BOOST_TEST_EQ(header.id, current_event_id);
    // missing check on payload

    // Update offsets and increment counters
    m_offset += sizeof(metadata);
    d_offset += header.length;
    ++current_event_id;
    --generated_events;
  }

  boost::report_errors();
}


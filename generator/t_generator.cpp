#include <iostream>

#include <cstdlib>

#include <boost/detail/lightweight_test.hpp>

#include "commons/dataformat.h"
#include "generator/Generator.h"

static size_t const gigabyte = 1073741824;
static size_t const megabyte = 1048576;

int main() {
  size_t const metadata_size = gigabyte;
  char* const metadata_ptr = new char[metadata_size];

  size_t const data_size = gigabyte;
  char* const data_ptr = new char[data_size];

  Generator generator(metadata_ptr, metadata_size, data_ptr, data_size);

  int generated_events = generator.generateEvents();

  std::cout << "Generated " << generated_events << " events." << std::endl;

  // Now check consistency between metadata and data

  size_t m_offset = 0;
  size_t d_offset = 0;

  uint64_t current_event_id = 0;

  while (generated_events != 0) {
    // Check MetaData
    MetaData const& metadata = *metadata_cast(metadata_ptr + m_offset);
    BOOST_TEST_EQ(metadata.id, current_event_id);
    BOOST_TEST_EQ(metadata.length, sizeof(Data));
    BOOST_TEST_EQ(metadata.offset, d_offset);

    // Check Data
    Data const& data = *data_cast(data_ptr + d_offset);
    BOOST_TEST_EQ(data.length, sizeof(Data));
    BOOST_TEST_EQ(data.flags, 0);
    BOOST_TEST_EQ(data.id, current_event_id);
    // missing check on payload

    // Update offsets and increment counters
    m_offset += sizeof(metadata);
    d_offset += sizeof(data);
    current_event_id++;
    generated_events--;
  }

  delete[] metadata_ptr;
  delete[] data_ptr;

  boost::report_errors();
}


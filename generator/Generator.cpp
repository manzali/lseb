#include <cstring> // memset

#include "generator/Generator.h"
#include "commons/dataformat.h"

bool checkMetaDataSize(size_t size, size_t offset) {
  return offset + sizeof(MetaData) > size ? false : true;
}

bool checkDataSize(size_t size, size_t offset) {
  return offset + sizeof(Data) > size ? false : true;
}

Generator::Generator(char* metadata_ptr, size_t metadata_size, char* data_ptr,
                     size_t data_size)
    : current_event_id_(0),
      metadata_ptr_(metadata_ptr),
      metadata_size_(metadata_size),
      data_ptr_(data_ptr),
      data_size_(data_size) {
}

int Generator::generateEvents() {

  size_t m_offset = 0;
  size_t d_offset = 0;

  uint64_t const starting_event_id = current_event_id_;

  while (checkMetaDataSize(metadata_size_, m_offset)
      && (checkDataSize(data_size_, d_offset))) {
    // Set MetaData
    MetaData& metadata = *metadata_cast(metadata_ptr_ + m_offset);
    metadata.id = current_event_id_;
    metadata.length = sizeof(Data);
    metadata.offset = d_offset;

    // Set Data
    Data& data = *data_cast(data_ptr_ + d_offset);
    data.length = sizeof(Data);
    data.flags = 0;
    data.id = current_event_id_;
    memset(data.payload, 0, payload_size);

    // Update offsets and increment counters
    m_offset += sizeof(metadata);
    d_offset += sizeof(data);
    current_event_id_++;
  }

  return current_event_id_ - starting_event_id;
}

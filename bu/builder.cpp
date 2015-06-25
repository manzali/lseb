#include "bu/builder.h"

#include "common/log.hpp"

namespace lseb {

void checkData(iovec const& iov) {
  size_t bytes_parsed = 0;
  uint64_t expected_event_id = pointer_cast<EventHeader>(iov.iov_base)->id;
  bool warning = false;
  while (bytes_parsed < iov.iov_len) {
    uint64_t current_event_id = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->id;
    uint64_t current_event_length = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->length;
    uint64_t current_event_flags = pointer_cast<EventHeader>(
      static_cast<char*>(iov.iov_base) + bytes_parsed)->flags;

    if (expected_event_id != current_event_id) {
      if (warning) {
        // Print event header
        LOG(WARNING)
          << "Error parsing EventHeader:"
          << std::endl
          << "expected event id: "
          << expected_event_id
          << std::endl
          << "event id: "
          << current_event_id
          << std::endl
          << "event length: "
          << current_event_length
          << std::endl
          << "event flags: "
          << current_event_flags;

        // terminate parsing
        return;

      } else {
        // if the event id is different from the expected one, check the next
        warning = true;
      }
    } else {
      warning = false;
    }
    expected_event_id = ++current_event_id;
    bytes_parsed += current_event_length;
  }
}

Builder::Builder(int n_threads)
    :
      m_executor(n_threads) {
}

void Builder::build(std::vector<iovec> total_iov) {
  // Check data
  for (auto& iov : total_iov) {
    m_executor.post(std::bind(checkData, iov));
  }
  m_executor.wait();
}

}

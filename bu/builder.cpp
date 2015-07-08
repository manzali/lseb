#include "bu/builder.h"

#include "common/log.hpp"

namespace lseb {

void checkData(int conn, iovec const& iov) {
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
    if (conn != current_event_flags || expected_event_id != current_event_id) {
      if (conn != current_event_flags || warning) {
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
      }
      warning = true;
    } else {
      warning = false;
    }
    expected_event_id = ++current_event_id;
    bytes_parsed += current_event_length;
  }
}

Builder::Builder() {
}

void Builder::build(int conn, std::vector<iovec> const& conn_iov) {
  // Check data
  for (auto const& iov : conn_iov) {
    checkData(conn, iov);
  }
}

}

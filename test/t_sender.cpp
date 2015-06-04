#include <iostream>

#include <cstring>
#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 3 && "sender <ip_address> <buffer_size>");

  // Connect to the receiver
  RuConnectionId conn = lseb_connect(argv[1], 7123);

  std::cout << "Connection established\n";

  // Sync with the receiver
  lseb_sync(conn);

  std::cout << "Sync done\n";

  size_t buffer_size = std::stol(argv[2]);
  char* buffer = new char[buffer_size];
  memset(buffer, '0', buffer_size);

  std::vector<iovec> iov;
  // sizeof(size_t) is for avail field
  iov.push_back( { buffer, buffer_size - sizeof(size_t)});

  FrequencyMeter bandwith(1.0);

  while (true) {
    if (lseb_poll(conn)) {
      ssize_t ret = lseb_write(conn, iov);
      assert(ret != -1);
      if (ret != -2) {
        bandwith.add(ret);
      }
    }
    if (bandwith.check()) {
      std::cout
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s\n"
        << std::flush;
    }
  }

  return 0;
}

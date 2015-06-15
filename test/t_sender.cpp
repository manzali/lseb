#include <iostream>

#include <cstring>
#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 3 && "sender <ip_address> <port> <transfer_size>");

  long port = std::stol(argv[2]);

  // Connect to the receiver
  RuConnectionId conn = lseb_connect(argv[1], port);

  std::cout << "Connection established\n";

  // Sync with the receiver
  lseb_sync(conn);

  std::cout << "Sync done\n";

  size_t transfer_size = std::stol(argv[3]);

  char* buffer = new char[transfer_size];
  memset(buffer, '0', transfer_size);

  std::vector<iovec> iov;
  iov.push_back( { buffer, transfer_size});

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
        << bandwith.frequency() / 134217728.
        << " Gb/s\n"
        << std::flush;
    }
  }

  return 0;
}

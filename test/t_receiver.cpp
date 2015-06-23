#include <iostream>

#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 4 && "receiver <ip_address> <port> <buffer_size>");

  long port = std::stol(argv[2]);

  // Create socket
  BuSocket socket = lseb_listen(argv[1], port);

  size_t buffer_size = std::stol(argv[3]);
  char* buffer = new char[buffer_size];

  // Accept connection from sender
  BuConnectionId conn = lseb_accept(socket, buffer, buffer_size);

  std::cout << "Connection established\n";

  // Sync with the sender
  lseb_sync(conn);

  std::cout << "Sync done\n";

  FrequencyMeter bandwith(1.0);

  while (true) {
    if (lseb_poll(conn)) {
      std::vector<iovec> iov = lseb_read(conn);
      for (auto& i : iov) {
        bandwith.add(i.iov_len);
      }
      lseb_release(conn);
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

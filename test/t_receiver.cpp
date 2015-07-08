#include <iostream>

#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 5 && "receiver <ip_address> <port> <buffer_size> <tokens>");

  // Create socket
  BuSocket socket = lseb_listen(argv[1], argv[2], std::stol(argv[4]));

  size_t buffer_size = std::stol(argv[3]);
  char* buffer = new char[buffer_size];

  // Accept connection from sender
  BuConnectionId conn = lseb_accept(socket);

  std::cout << "Connection established\n";

  lseb_register(conn, buffer, buffer_size);

  FrequencyMeter bandwith(1.0);

  while (true) {
    std::vector<iovec> iov = lseb_read(conn);
    if (iov.size()) {
      std::vector<void*> wrs;
      for (auto& i : iov) {
        bandwith.add(i.iov_len);
        wrs.push_back(i.iov_base);
      }
      lseb_release(conn, wrs);
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

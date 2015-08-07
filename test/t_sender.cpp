#include <iostream>

#include <cstring>
#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 5 && "sender <ip_address> <port> <transfer_size> <tokens>");

  // Connect to the receiver
  RuConnectionId conn = lseb_connect(argv[1], argv[2], std::stol(argv[4]));

  std::cout << "Connection established\n";

  size_t transfer_size = std::stol(argv[3]);

  char* buffer = new char[transfer_size];
  memset(buffer, '0', transfer_size);

  DataIov iov;
  iov.push_back( { buffer, transfer_size });
  std::vector<DataIov> data_iovecs;
  data_iovecs.push_back(iov);

  lseb_register(conn, 0, buffer, transfer_size);

  FrequencyMeter bandwith(1.0);

  while (true) {
    if (lseb_avail(conn)) {
      ssize_t ret = lseb_write(conn, data_iovecs);
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

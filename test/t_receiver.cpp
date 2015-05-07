#include <iostream>

#include <cassert>

#include "transport/transport.h"
#include "common/frequency_meter.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  // Check arguments
  assert(argc == 2 && "sender <ip_address>");

  // Create socket
  BuSocket socket = lseb_listen(argv[1], 7123);

  size_t buffer_size = 1024 * 1024;
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
      ssize_t ret = lseb_read(conn);
      assert(ret != -1);
      bandwith.add(ret);
      lseb_release(conn);
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

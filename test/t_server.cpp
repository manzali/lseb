#include <iostream>

#include <boost/program_options.hpp>

#include "common/memory_pool.h"
#include "common/frequency_meter.h"

#include "transport/transport.h"

using namespace lseb;

int main(int argc, char* argv[]) {

  boost::program_options::options_description desc("Options");

  std::string server;
  std::string port;
  size_t chunk_size;
  int credits;

  desc.add_options()("help,h", "Print help messages.")(
    "server,s",
    boost::program_options::value<std::string>(&server)->required(),
    "Server.")(
    "port,p",
    boost::program_options::value<std::string>(&port)->required(),
    "Port.")(
    "chunk_size,c",
    boost::program_options::value<size_t>(&chunk_size)->required(),
    "Buffer size.")(
    "credits,C",
    boost::program_options::value<int>(&credits)->required(),
    "Credits.");

  try {
    boost::program_options::variables_map vm;
    boost::program_options::store(
      boost::program_options::command_line_parser(argc, argv).options(desc).run(),
      vm);
    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return EXIT_SUCCESS;
    }
    boost::program_options::notify(vm);
  } catch (const boost::program_options::error& e) {
    std::cerr << e.what() << std::endl << desc << std::endl;
    return EXIT_FAILURE;
  }

  std::cout
    << "Allocating "
    << chunk_size * credits
    << " bytes of memory"
    << std::endl;

  std::unique_ptr<unsigned char[]> const buffer_ptr(
    new unsigned char[chunk_size * credits]);

  MemoryPool pool(buffer_ptr.get(), chunk_size, credits);

  Acceptor<RecvSocket> acceptor(credits);

  acceptor.listen(server, port);
  RecvSocket socket = acceptor.accept();
  socket.register_memory(buffer_ptr.get(), chunk_size * credits);
  std::cout << "Accepted connection" << std::endl;

  FrequencyMeter bandwith(5.0);

  while (!pool.empty()) {
    socket.post_read(pool.alloc());
  }

  while (true) {
    std::vector<iovec> vect = socket.pop_completed();
    for (auto& iov : vect) {
      pool.free(iov);
    }
    bandwith.add(iovec_length(vect));
    while (!pool.empty()) {
      socket.post_read(pool.alloc());
    }
    if (bandwith.check()) {
      std::cout
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s"
        << std::endl;
    }
  }
}

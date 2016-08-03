#include <iostream>

#include <boost/program_options.hpp>

#include "common/memory_pool.h"
#include "common/frequency_meter.h"
#include "common/utility.h"
#include "common/log.hpp"

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

  Log::init("t_server2", Log::INFO);

  size_t const buffer_size = chunk_size * credits;

  LOG(INFO) << "Two-way server test";

  std::unique_ptr<unsigned char[]> const send_buffer_ptr(
    new unsigned char[buffer_size]);
  std::unique_ptr<unsigned char[]> const recv_buffer_ptr(
    new unsigned char[buffer_size]);

  MemoryPool send_pool(send_buffer_ptr.get(), buffer_size, chunk_size);
  MemoryPool recv_pool(recv_buffer_ptr.get(), buffer_size, chunk_size);

  Acceptor acceptor(credits);

  acceptor.listen(server, port);
  std::unique_ptr<Socket> socket = acceptor.accept();
  socket->register_memory(send_buffer_ptr.get(), buffer_size);
  socket->register_memory(recv_buffer_ptr.get(), buffer_size);

  LOG(INFO) << "Accepted connection";

  FrequencyMeter bandwith(5.0);

  while (!recv_pool.empty()) {
    socket->post_recv(recv_pool.alloc());
  }

  while (true) {
    // Recv
    std::vector<iovec> recv_vect = socket->poll_completed_recv();
    for (auto& iov : recv_vect) {
      recv_pool.free(iov);
    }
    bandwith.add(iovec_length(recv_vect));
    if (!recv_pool.empty()) {
      socket->post_recv(recv_pool.alloc());
    }

    // Send
    std::vector<iovec> send_vect = socket->poll_completed_send();
    for (auto& iov : send_vect) {
      bandwith.add(iov.iov_len);
      send_pool.free({iov.iov_base, chunk_size});
    }
    if (socket->available_send()) {
      socket->post_send(send_pool.alloc());
    }

    if (bandwith.check()) {
      LOG(INFO)
        << "Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
    }
  }
}

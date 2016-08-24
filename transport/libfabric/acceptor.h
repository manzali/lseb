#ifndef TRANSPORT_LIBFABRIC_ACCEPTOR_H
#define TRANSPORT_LIBFABRIC_ACCEPTOR_H

#include <memory>

#include <rdma/fabric.h>

#include "transport/libfabric/socket.h"
#include "transport/libfabric/domain.h"

namespace lseb {

class Acceptor {
public:

  typedef std::unique_ptr<fid_pep, fid_deleter<fid_pep>> pep_ptr;
  typedef std::unique_ptr<fid_eq, fid_deleter<fid_eq>> eq_ptr;

  Acceptor(int credits);
  Acceptor(Acceptor const& other) = delete;
  Acceptor& operator=(Acceptor const&) = delete;
  Acceptor(Acceptor&& other) = default;
  Acceptor& operator=(Acceptor&&) = default;

  void listen(std::string const& hostname, std::string const& port);
  ~Acceptor() = default;
  std::unique_ptr<Socket> accept();

private:
  uint32_t m_credits;
  pep_ptr m_pep;
  eq_ptr m_pep_eq;

};

}

#endif

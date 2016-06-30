#ifndef BU_BUILDER_UNIT_H
#define BU_BUILDER_UNIT_H

#include <map>

#include <sys/uio.h>

#include "transport/transport.h"
#include "transport/endpoints.h"

namespace lseb {

class BuilderUnit {
  std::vector<Endpoint> m_endpoints;
  std::map<int, std::unique_ptr<RecvSocket> > m_connection_ids;
  std::vector<std::vector<iovec> > m_data_vect;
  int m_bulk_size;
  int m_credits;
  int m_max_fragment_size;
  int m_id;

  std::unique_ptr<unsigned char[]> m_data_ptr;

  int read_data(int id);
  bool check_data();
  size_t release_data(int id, int n);

 public:
  BuilderUnit(
    std::vector<Endpoint> const& endpoints,
    int bulk_size,
    int credits,
    int max_fragment_size,
    int id);
  void connect();
  void run();
};

}

#endif

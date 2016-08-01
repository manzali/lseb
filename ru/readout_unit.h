#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include <map>

#include <sys/uio.h>

#include "ru/accumulator.h"

#include "transport/transport.h"
#include "transport/endpoints.h"

namespace lseb {

class ReadoutUnit {
  Accumulator& m_accumulator;
  std::vector<Endpoint> m_endpoints;
  std::map<int, std::unique_ptr<Socket> > m_connection_ids;
  int m_bulk_size;
  int m_credits;
  int m_id;

 public:
  ReadoutUnit(
    Accumulator& accumulator,
    std::vector<Endpoint> const& endpoints,
    int bulk_size,
    int credits,
    int id);
  void connect();
  void run();
};

}

#endif

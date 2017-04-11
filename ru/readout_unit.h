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
  std::map<int, std::unique_ptr<Socket> > m_connection_ids;
  int m_credits;
  int m_id;

 public:
  ReadoutUnit(
    Accumulator& accumulator,
    int credits,
    int id);
  void connect(std::vector<Endpoint> const& endpoints);
  void run();
};

}

#endif

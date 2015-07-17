#ifndef RU_READOUT_UNIT_H
#define RU_READOUT_UNIT_H

#include "common/configuration.h"

namespace lseb {

class ReadoutUnit {
  Configuration m_configuration;
  int m_id;

 public:
  ReadoutUnit(Configuration const& configuration, int id);
  void operator()();
};

}

#endif

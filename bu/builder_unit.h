#ifndef BU_BUILDER_UNIT_H
#define BU_BUILDER_UNIT_H

#include "common/configuration.h"

namespace lseb {

class BuilderUnit {
  Configuration m_configuration;
  int m_id;

 public:
  BuilderUnit(Configuration const& configuration, int id);
  void operator()();
};

}

#endif

#ifndef RU_CONTROLLER_H
#define RU_CONTROLLER_H

#include <cmath>

#include "../common/dataformat.h"
#include "../common/shared_queue.h"
#include "generator/generator.h"

namespace lseb {

class Controller {

  Generator m_generator;
  MetaDataRange m_metadata_range;
  SharedQueue<MetaDataRange>& m_ready_events_queue;
  SharedQueue<MetaDataRange>& m_sent_events_queue;

 public:
  Controller(Generator const& generator, MetaDataRange const& metadata_range,
             SharedQueue<MetaDataRange>& ready_events_queue,
             SharedQueue<MetaDataRange>& sent_events_queue);
  void operator()(size_t frequency);

};

}

#endif

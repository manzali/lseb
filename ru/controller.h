#ifndef RU_CONTROLLER_H
#define RU_CONTROLLER_H

#include <cmath>

#include "commons/dataformat.h"
#include "commons/shared_queue.h"

#include "generator/generator.h"

namespace lseb {

typedef SharedQueue<EventMetaDataRange> EventsQueue;

class Controller {

 private:
  Generator m_generator;
  EventMetaData* const m_begin_metadata;
  EventMetaData* const m_end_metadata;
  EventsQueue& m_ready_events_queue;
  EventsQueue& m_sent_events_queue;

 public:
  Controller(Generator const& generator, EventMetaData* begin_metadata,
             EventMetaData* end_metadata, EventsQueue& ready_events_queue,
             EventsQueue& sent_events_queue);
  void operator()(size_t frequency);

};

}

#endif

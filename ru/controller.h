#ifndef RU_CONTROLLER_H
#define RU_CONTROLLER_H

#include <memory>

#include <cmath>

#include "commons/dataformat.h"
#include "commons/shared_queue.h"

#include "generator/generator.h"

namespace lseb {

typedef SharedQueue<EventMetaDataPair> EventsQueue;
typedef std::shared_ptr<EventsQueue> EventsQueuePtr;

class Controller {

 private:
  Generator m_generator;
  EventMetaData* const m_begin_metadata;
  EventMetaData* const m_end_metadata;
  EventsQueuePtr m_ready_events_queue;
  EventsQueuePtr m_sent_events_queue;

 public:
  Controller(Generator const& generator, EventMetaData* begin_metadata,
             EventMetaData* end_metadata, EventsQueuePtr ready_events_queue,
             EventsQueuePtr sent_events_queue);
  void operator()(size_t frequency);

};

}

#endif

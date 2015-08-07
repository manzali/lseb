#include <memory>
#include <string>
#include <map>

#include <cstdlib>
#include <cassert>

#include "common/dataformat.h"
#include "common/log.hpp"
#include "common/utility.h"
#include "common/configuration.h"
#include "common/frequency_meter.h"
#include "common/timer.h"

#include "generator/generator.h"
#include "generator/length_generator.h"

#include "ru/controller.h"
#include "ru/accumulator.h"
#include "ru/readout_unit.h"

#include "transport/transport.h"
#include "transport/endpoints.h"

namespace lseb {

ReadoutUnit::ReadoutUnit(
  Configuration const& configuration,
  int id,
  boost::lockfree::spsc_queue<iovec>& free_local_data,
  boost::lockfree::spsc_queue<iovec>& ready_local_data)
    :
      m_configuration(configuration),
      m_free_local_data(free_local_data),
      m_ready_local_data(ready_local_data),
      m_id(id) {
}

void ReadoutUnit::operator()() {

  int const generator_frequency = m_configuration.get<int>(
    "GENERATOR.FREQUENCY");
  assert(generator_frequency > 0);

  int const mean = m_configuration.get<int>("GENERATOR.MEAN");
  assert(mean > 0);

  int const stddev = m_configuration.get<int>("GENERATOR.STD_DEV");
  assert(stddev >= 0);

  int const max_fragment_size = m_configuration.get<int>(
    "GENERAL.MAX_FRAGMENT_SIZE");
  assert(max_fragment_size > 0);

  int const bulk_size = m_configuration.get<int>("GENERAL.BULKED_EVENTS");
  assert(bulk_size > 0);

  int const tokens = m_configuration.get<int>("GENERAL.TOKENS");
  assert(tokens > 0);

  int const meta_size = m_configuration.get<int>("RU.META_BUFFER");
  assert(meta_size > 0);

  int const data_size = m_configuration.get<int>("RU.DATA_BUFFER");
  assert(data_size > 0);

  std::vector<Endpoint> const endpoints = get_endpoints(
    m_configuration.get_child("ENDPOINTS"));

  assert(
    meta_size % sizeof(EventMetaData) == 0 && "wrong metadata buffer size");

  // Allocate memory

  std::unique_ptr<unsigned char[]> const metadata_ptr(
    new unsigned char[meta_size]);
  std::unique_ptr<unsigned char[]> const data_ptr(new unsigned char[data_size]);

  MetaDataRange metadata_range(
    pointer_cast<EventMetaData>(metadata_ptr.get()),
    pointer_cast<EventMetaData>(metadata_ptr.get() + meta_size));
  DataRange data_range(data_ptr.get(), data_ptr.get() + data_size);

  MetaDataBuffer metadata_buffer(
    std::begin(metadata_range),
    std::end(metadata_range));
  DataBuffer data_buffer(std::begin(data_range), std::end(data_range));

  int next_id = m_id;
  std::vector<int> id_sequence(endpoints.size());
  for (auto& id : id_sequence) {
    next_id = (next_id != endpoints.size() - 1) ? next_id + 1 : 0;
    id = next_id;
  }

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";
  std::map<int, RuConnectionId> connection_ids;
  for (auto id : id_sequence) {
    if (id != m_id) {
      Endpoint const& ep = endpoints.at(id);
      auto p = connection_ids.emplace(
        id,
        lseb_connect(ep.hostname(), ep.port(), tokens));
      RuConnectionId& conn = p.first->second;
      lseb_register(conn, m_id, data_ptr.get(), data_size);
      LOG(NOTICE) << "Readout Unit - Connection established with bu " << id;
    }
  }
  LOG(NOTICE) << "Readout Unit - All connections established";

  Accumulator accumulator(metadata_range, data_range, bulk_size);

  LengthGenerator payload_size_generator(mean, stddev, max_fragment_size);
  Generator generator(
    payload_size_generator,
    metadata_buffer,
    data_buffer,
    m_id);
  Controller controller(generator, metadata_range, generator_frequency);

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_ctrl;
  Timer t_send;

  unsigned int const needed_events = bulk_size * endpoints.size();
  unsigned int const needed_multievents = endpoints.size();

  while (true) {

    t_ctrl.start();
    unsigned int ready_events = 0;
    do {
      ready_events = accumulator.add(controller.read());
    } while (needed_events >= ready_events);
    std::vector<MultiEvent> multievents = accumulator.get_multievents(
      needed_multievents);
    t_ctrl.pause();

    // Fill data_vect
    std::vector<DataIov> data_vect;
    for (auto const& multievent : multievents) {
      data_vect.emplace_back(create_iovec(multievent.second, data_range));
    }

    assert(data_vect.size() == id_sequence.size());

    t_send.start();
    for (auto id : id_sequence) {
      auto& data_iov = data_vect.at(id);
      if (id != m_id) {
        auto conn_it = connection_ids.find(id);
        assert(conn_it != std::end(connection_ids));
        auto& conn = conn_it->second;
        while (!lseb_avail(conn)) {
          ;
        }
        bandwith.add(lseb_write(conn, std::vector<DataIov>(1, data_iov)));
      } else {
        iovec i;
        while (!m_free_local_data.pop(i)) {
          ;
        }
        i.iov_len = 0;
        for (auto& iov : data_iov) {
          memcpy(
            static_cast<unsigned char*>(i.iov_base) + i.iov_len,
            iov.iov_base,
            iov.iov_len);
          i.iov_len += iov.iov_len;
        }
        while (!m_ready_local_data.push(i)) {
          ;
        }
      }
      LOG(DEBUG) << "Written " << data_iov.size() << " wr to conn " << id;
    }
    t_send.pause();

    t_ctrl.start();
    controller.release(
      MetaDataRange(
        std::begin(multievents.front().first),
        std::end(multievents.back().first)));
    t_ctrl.pause();

    frequency.add(multievents.size() * bulk_size);

    if (frequency.check()) {
      LOG(NOTICE)
        << "Readout Unit: "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_ctrl: "
        << t_ctrl.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%";
      t_ctrl.reset();
      t_send.reset();
    }
  }
}

}

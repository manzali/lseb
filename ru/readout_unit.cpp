#include <memory>
#include <string>
#include <cstdlib>
#include <cassert>
#include <algorithm>

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
#include "ru/splitter.h"
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

  int next_id = (m_id + 1 == endpoints.size()) ? 0 : m_id + 1;
  std::vector<int> id_sequence(endpoints.size());
  for (auto& id : id_sequence) {
    id = next_id;
    if (++next_id == endpoints.size()) {
      next_id = 0;
    }
  }

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";
  std::map<int, RuConnectionId> connection_ids;
  for (auto id : id_sequence) {
    if (id != m_id) {
      connection_ids.emplace(
        id,
        lseb_connect(endpoints[id].hostname(), endpoints[id].port(), tokens));
    }
  }
  LOG(NOTICE) << "Readout Unit - All connections established";

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

  Accumulator accumulator(metadata_range, data_range, bulk_size);

  for (auto& conn : connection_ids) {
    lseb_register(conn.second, data_ptr.get(), data_size);
  }

  LengthGenerator payload_size_generator(mean, stddev, max_fragment_size);
  Generator generator(
    payload_size_generator,
    metadata_buffer,
    data_buffer,
    m_id);
  Controller controller(generator, metadata_range, generator_frequency);

  Splitter splitter(m_id, endpoints.size(), data_range);

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0); // this timeout is ignored (frequency is used)

  Timer t_accu;
  Timer t_send;

  unsigned int const needed_events = bulk_size * endpoints.size();
  unsigned int const needed_multievents = endpoints.size();

  while (true) {

    t_accu.start();
    unsigned int ready_events = 0;
    do {
      ready_events = accumulator.add(controller.read());
    } while (needed_events >= ready_events);
    std::vector<MultiEvent> multievents = accumulator.get_multievents(
      needed_multievents);
    t_accu.pause();

    assert(multievents.size() == needed_multievents);

    std::map<int, std::vector<DataIov> > iov_map = splitter.split(multievents);

    t_send.start();
    for (auto id : id_sequence) {
      auto map_it = iov_map.find(id);
      assert(map_it != std::end(iov_map));
      auto& iov_pair = *map_it;
      if (id != m_id) {
        auto it = connection_ids.find(iov_pair.first);
        assert(it != std::end(connection_ids));
        while (lseb_avail(it->second) < iov_pair.second.size()) {
          ;
        }
        size_t bytes = lseb_write(it->second, iov_pair.second);
        bandwith.add(bytes);
      } else {
        for (auto& data_iov : iov_pair.second) {
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
      }
      LOG(DEBUG)
        << "Written "
        << iov_pair.second.size()
        << " wr to conn "
        << id;
    }
    t_send.pause();

    t_accu.start();
    controller.release(
      MetaDataRange(
        std::begin(multievents.front().first),
        std::end(multievents.back().first)));
    t_accu.pause();

    frequency.add(multievents.size() * bulk_size);
/*
    if (bandwith.check()) {
      LOG(NOTICE)
        << "Readout Unit - Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
    }
*/
    if (frequency.check()) {
      /*
      LOG(NOTICE)
        << "Readout Unit - Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";
      */

      LOG(NOTICE)
        << "Readout Unit - Frequency "
        << frequency.frequency() / std::mega::num
        << " MHz - "
        << bandwith.frequency() / std::giga::num * 8.
        << "Gb/s";

      LOG(INFO)
        << "Times:\n"
        << "\tt_accu: "
        << t_accu.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%";
      t_accu.reset();
      t_send.reset();
    }
  }
}

}

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
  assert(stddev > 0);

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

  std::chrono::milliseconds ms_timeout(
    m_configuration.get<int>("RU.MS_TIMEOUT"));

  std::vector<Endpoint> const endpoints = get_endpoints(
    m_configuration.get_child("ENDPOINTS"));

  assert(
    meta_size % sizeof(EventMetaData) == 0 && "wrong metadata buffer size");

  int start_id = (m_id + 1 == endpoints.size()) ? 0 : m_id + 1;
  int wrap_id = endpoints.size() - 1;

  LOG(NOTICE) << "Waiting for connections...";
  std::map<int, RuConnectionId> connection_ids;
  for (int i = start_id; i != m_id; i = (i == wrap_id) ? 0 : i + 1) {
    connection_ids.emplace(
      i,
      lseb_connect(endpoints[i].hostname(), endpoints[i].port(), tokens));
  }
  LOG(NOTICE) << "Connections established";

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
  FrequencyMeter bandwith(5.0);

  Timer t_accu;
  Timer t_send;
  Timer t_spli;

  int const mul = m_configuration.get<int>("GENERAL.MUL");
  assert(mul > 0);

  unsigned int const needed_events = mul * bulk_size * endpoints.size();
  unsigned int const needed_multievents = mul * endpoints.size();

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

    t_spli.start();
    std::map<int, std::vector<DataIov> > iov_map = splitter.split(multievents);
    t_spli.pause();

    t_send.start();

    for (int i = start_id; i != m_id; i = (i == wrap_id) ? 0 : i + 1) {
      auto map_it = iov_map.find(i);
      assert(map_it != std::end(iov_map));
      auto& iov_pair = *map_it;
      assert(iov_pair.second.size() == mul);
      auto it = connection_ids.find(iov_pair.first);

      while (lseb_avail(it->second) < mul) {
        ;
      }
      // time
      size_t bytes = lseb_write(it->second, iov_pair.second);
      // time
      LOG(DEBUG)
        << "Written "
        << iov_pair.second.size()
        << " wr to conn "
        << it->first;
      bandwith.add(bytes);
    }

    auto it = iov_map.find(m_id);
    assert(it != std::end(iov_map));
    auto& iov_pair = *it;
    assert(iov_pair.second.size() == mul);

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
      bandwith.add(i.iov_len);
      while (!m_ready_local_data.push(i)) {
        ;
      }
    }
    LOG(DEBUG)
      << "Written "
      << iov_pair.second.size()
      << " wr to conn "
      << m_id;
    t_send.pause();

    t_accu.start();
    controller.release(
      MetaDataRange(
        std::begin(multievents.front().first),
        std::end(multievents.back().first)));
    t_accu.pause();

    frequency.add(multievents.size() * bulk_size);

    if (bandwith.check()) {
      LOG(NOTICE)
        << "Readout Unit - Bandwith: "
        << bandwith.frequency() / std::giga::num * 8.
        << " Gb/s";
    }

    if (frequency.check()) {
      LOG(NOTICE)
        << "Readout Unit - Frequency: "
        << frequency.frequency() / std::mega::num
        << " MHz";

      LOG(INFO)
        << "Times:\n"
        << "\tt_accu: "
        << t_accu.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%\n"
        << "\tt_spli: "
        << t_spli.rate()
        << "%";
      t_accu.reset();
      t_send.reset();
      t_spli.reset();
    }
  }
}

}

#include <memory>
#include <string>
#include <map>
#include <thread>
#include <chrono>
#include <algorithm>

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

#include "transport/endpoints.h"

namespace lseb {

std::vector<int> create_sequence(int id, int total) {
  int first_id = (id != total - 1) ? id + 1 : 0;
  std::vector<int> sequence(total);
  std::iota(std::begin(sequence), std::end(sequence), 0);
  std::rotate(
    std::begin(sequence),
    std::begin(sequence) + first_id,
    std::end(sequence));
  return sequence;
}

ReadoutUnit::ReadoutUnit(
  Configuration const& configuration,
  int id,
  boost::lockfree::spsc_queue<iovec>& free_local_data,
  boost::lockfree::spsc_queue<iovec>& ready_local_data)
    :
      m_configuration(configuration),
      m_free_local_queue(free_local_data),
      m_ready_local_queue(ready_local_data),
      m_id(id) {
}

size_t ReadoutUnit::remote_write(int id, iovec const& iov) {
  auto conn_it = m_connection_ids.find(id);
  assert(conn_it != std::end(m_connection_ids));
  auto& conn = conn_it->second;
  return conn.post_write(iov);
}

size_t ReadoutUnit::local_write(iovec const& iov) {
  iovec i;
  while (!m_free_local_queue.pop(i)) {
    ;
  }
  memcpy(static_cast<unsigned char*>(i.iov_base), iov.iov_base, iov.iov_len);
  i.iov_len = iov.iov_len;
  while (!m_ready_local_queue.push(i)) {
    ;
  }
  m_local_data.push_back(iov);
  return i.iov_len;
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
  assert(max_fragment_size >= sizeof(EventHeader));

  int const bulk_size = m_configuration.get<int>("GENERAL.BULKED_EVENTS");
  assert(bulk_size > 0);

  int const credits = m_configuration.get<int>("GENERAL.CREDITS");
  assert(credits > 0);

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

  int const required_multievents = endpoints.size();

  std::vector<int> id_sequence = create_sequence(m_id, required_multievents);

  LOG(NOTICE) << "Readout Unit - Waiting for connections...";

  Connector<SendSocket> connector(data_ptr.get(), data_size, credits);

  for (auto id : id_sequence) {
    if (id != m_id) {
      Endpoint const& ep = endpoints.at(id);
      bool connected = false;
      while (!connected) {
        try {
          m_connection_ids.emplace(
            std::make_pair(id, connector.connect(ep.hostname(), ep.port())));
          connected = true;
        } catch (std::exception& e) {
          std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
      }
      LOG(NOTICE)
        << "Readout Unit - Connection established with bu "
        << id
        << ")";
    }
  }
  LOG(NOTICE) << "Readout Unit - All connections established";

  LengthGenerator payload_size_generator(
    mean,
    stddev,
    max_fragment_size - sizeof(EventHeader));
  Generator generator(payload_size_generator, metadata_range, data_range, m_id);
  Controller controller(generator, metadata_range, generator_frequency);
  Accumulator accumulator(
    controller,
    metadata_range,
    data_range,
    bulk_size,
    endpoints.size());

  FrequencyMeter frequency(5.0);
  FrequencyMeter bandwith(5.0);  // this timeout is ignored (frequency is used)

  Timer t_ctrl;
  Timer t_send;
  Timer t_release;

  while (true) {

    // Release
    t_release.start();
    std::vector<iovec> iov_to_release;
    for (auto id : id_sequence) {
      bool available = false;
      do {
        std::vector<iovec> partial;
        if (id != m_id) {
          auto conn_it = m_connection_ids.find(id);
          assert(conn_it != std::end(m_connection_ids));
          auto& conn = conn_it->second;
          partial = conn.pop_completed();
          available = (credits - conn.pending() != 0);
        } else {
          partial = m_local_data;
          m_local_data.clear();
          available = true;
        }
        if (!partial.empty()) {
          iov_to_release.insert(
            std::end(iov_to_release),
            std::begin(partial),
            std::end(partial));
          LOG(DEBUG)
            << "Readout Unit - Completed "
            << partial.size()
            << " iov to conn "
            << id;
        }
      } while (!available);
    }
    if (!iov_to_release.empty()) {
      accumulator.release_multievents(iov_to_release);
    }
    t_release.pause();

    // Acquire
    t_ctrl.start();
    std::vector<iovec> iov_to_send = accumulator.get_multievents();
    t_ctrl.pause();

    // Send
    if (!iov_to_send.empty()) {
      assert(iov_to_send.size() == required_multievents);
      t_send.start();
      for (auto id : id_sequence) {
        auto& iov = iov_to_send.at(id);
        assert(iov.iov_len <= max_fragment_size * bulk_size);
        if (id != m_id) {
          size_t bytes = remote_write(id, iov);
          bandwith.add(bytes);
        } else {
          local_write(iov);
        }
        LOG(DEBUG) << "Readout Unit - Writing iov to conn " << id;
      }
      t_send.pause();
      frequency.add(required_multievents * bulk_size);
    }

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
        << "\tt_release: "
        << t_release.rate()
        << "%\n"
        << "\tt_send: "
        << t_send.rate()
        << "%";
      t_ctrl.reset();
      t_release.reset();
      t_send.reset();
    }
  }
}

}

#ifndef COMMONS_FREQUENCY_METER_H
#define COMMONS_FREQUENCY_METER_H

#include <chrono>

#include <cassert>

namespace lseb {

class FrequencyMeter {
  double m_counter;
  double const m_timeout;
  std::chrono::high_resolution_clock::time_point m_start_time;

 public:
  FrequencyMeter(double seconds)
      : m_counter(0.0),
        m_timeout(seconds),
        m_start_time(std::chrono::high_resolution_clock::now()) {
    assert(m_timeout > 0.0);
  }
  void add(double value) {
    assert(value >= 0.0 && "Trying to add a non positive value");
    m_counter += value;
  }
  bool check() {
    auto end_time = std::chrono::high_resolution_clock::now();
    double const elapsed_seconds = std::chrono::duration_cast<
        std::chrono::nanoseconds>(end_time - m_start_time).count()
        / 1000000000.;
    return elapsed_seconds >= m_timeout ? true : false;
  }
  double frequency() {
    auto end_time = std::chrono::high_resolution_clock::now();
    double const elapsed_seconds = std::chrono::duration_cast<
        std::chrono::nanoseconds>(end_time - m_start_time).count()
        / 1000000000.;
    double const frequency = m_counter / elapsed_seconds;
    m_start_time = std::chrono::high_resolution_clock::now();
    m_counter = 0.0;
    return frequency;
  }

  FrequencyMeter(const FrequencyMeter&) = delete;            // disable copying
  FrequencyMeter& operator=(const FrequencyMeter&) = delete;  // disable assignment
};

}

#endif

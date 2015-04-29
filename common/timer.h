#ifndef COMMON_TIMER_H
#define COMMON_TIMER_H

#include <chrono>

namespace lseb {

class Timer {
  std::chrono::high_resolution_clock::time_point m_begin_time;
  std::chrono::high_resolution_clock::time_point m_start_time;
  std::chrono::high_resolution_clock::duration m_active_time;
  bool m_paused;

 public:
  Timer()
      :
        m_begin_time(std::chrono::high_resolution_clock::now()),
        m_active_time(std::chrono::high_resolution_clock::duration::zero()),
        m_paused(true) {
  }
  void start() {
    assert(m_paused == true && "It's already started");
    m_start_time = std::chrono::high_resolution_clock::now();
    m_paused = false;
  }
  void pause() {
    assert(m_paused == false && "It's already paused");
    m_active_time += std::chrono::high_resolution_clock::duration(
      std::chrono::high_resolution_clock::now() - m_start_time);
    m_paused = true;
  }

  std::chrono::high_resolution_clock::duration active_time() {
    if (m_paused) {
      return m_active_time;
    }
    return m_active_time + std::chrono::high_resolution_clock::duration(
      std::chrono::high_resolution_clock::now() - m_start_time);
  }

  std::chrono::high_resolution_clock::duration total_time() {
    return std::chrono::high_resolution_clock::duration(
      std::chrono::high_resolution_clock::now() - m_begin_time);
  }

  double rate() {
    return active_time() * 100. / total_time();
  }
  void reset() {
    m_begin_time = std::chrono::high_resolution_clock::now();
    m_active_time = std::chrono::high_resolution_clock::duration::zero();
    m_paused = true;
  }
  Timer(const Timer&) = delete;            // disable copying
  Timer& operator=(const Timer&) = delete;  // disable assignment
};

}

#endif

#ifndef TRANSPORT_TCP_SHARED_QUEUE_H
#define TRANSPORT_TCP_SHARED_QUEUE_H

#include <queue>
#include <utility>

#include <boost/thread.hpp>

template<typename T>
class SharedQueue {
  std::queue<T> m_queue;
  boost::mutex m_mutex;
  boost::condition_variable m_cond;

 public:

  void push(T const& msg) {
    boost::mutex::scoped_lock lock(m_mutex);
    m_queue.push(msg);
    m_cond.notify_one();
  }

  T pop() {
    boost::mutex::scoped_lock lock(m_mutex);
    while (m_queue.empty()) {
      m_cond.wait(lock);
    }
    T msg = m_queue.front();
    m_queue.pop();
    return msg;
  }

  std::pair<T, bool> pop_no_wait() {
    boost::mutex::scoped_lock lock(m_mutex);
    if (m_queue.empty()) {
      return std::make_pair(T(), false);
    }
    T msg = m_queue.front();
    m_queue.pop();
    return std::make_pair(msg, true);
  }

  size_t size() const {
    boost::mutex::scoped_lock lock(m_mutex);
    return m_queue.size();
  }
};

#endif


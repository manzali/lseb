#ifndef COMMON_SHARED_QUEUE_H
#define COMMON_SHARED_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class SharedQueue {

  std::queue<T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cond;
  bool m_closed;

 public:

  T pop() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    while (!m_closed && m_queue.empty()) {
      m_cond.wait(mlock);
    }
    if (m_closed && m_queue.empty()) {
      m_cond.notify_one();
      throw std::runtime_error(
          "Trying to retrieve an item from a closed queue.");
    }
    T item(m_queue.front());
    m_queue.pop();
    return item;
  }

  void push(const T& item) {
    assert(!m_closed && "Trying to insert item in a closed queue.");
    std::unique_lock<std::mutex> mlock(m_mutex);
    m_queue.push(item);
    m_cond.notify_one();
  }

  bool empty() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    return m_queue.empty();
  }

  void close() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    m_closed = true;
    m_cond.notify_one();
  }

  size_t size() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    return m_queue.size();
  }

  SharedQueue()
      : m_closed(false) {
  }

  SharedQueue(const SharedQueue&) = delete;             // disable copying
  SharedQueue& operator=(const SharedQueue&) = delete;  // disable assignment

};

#endif

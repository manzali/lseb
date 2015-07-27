#ifndef COMMON_SHARED_QUEUE_H
#define COMMON_SHARED_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace lseb {

template<typename T>
class SharedQueue {
  std::queue<T> m_queue;
  std::mutex m_mutex;
  std::condition_variable m_cond;

 public:

  T pop() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    while (m_queue.empty()) {
      m_cond.wait(mlock);
    }
    T item = m_queue.front();
    m_queue.pop();
    return item;
  }

  bool pop_nowait(T& item) {
    std::unique_lock<std::mutex> mlock(m_mutex);
    if(!m_queue.empty()){
      item = m_queue.front();
      m_queue.pop();
      return true;
    }
    return false;
  }

  void push(T const& item) {
    std::unique_lock<std::mutex> mlock(m_mutex);
    m_queue.push(item);
    mlock.unlock();
    m_cond.notify_one();
  }

  size_t size() {
    std::unique_lock<std::mutex> mlock(m_mutex);
    return m_queue.size();
  }

  SharedQueue() = default;
  SharedQueue(const SharedQueue&) = delete;
  SharedQueue& operator=(const SharedQueue&) = delete;
};

}

#endif

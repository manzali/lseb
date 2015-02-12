#ifndef SHARED_QUEUE_H
#define SHARED_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

template<typename T>
class SharedQueue {

 private:
  std::queue<T> queue_;
  std::mutex mutex_;
  std::condition_variable cond_;

 public:

  bool pop_nowait(T& item) {
    std::unique_lock < std::mutex > mlock(mutex_);
    if (!queue_.empty()) {
      item = queue_.front();
      queue_.pop();
      return true;
    }
    return false;
  }

  void pop(T& item) {
    std::unique_lock < std::mutex > mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    item = queue_.front();
    queue_.pop();
  }

  void push(const T& item) {
    std::unique_lock < std::mutex > mlock(mutex_);
    queue_.push(item);
    mlock.unlock();
    cond_.notify_one();
  }
  SharedQueue() = default;
  SharedQueue(const SharedQueue&) = delete;            // disable copying
  SharedQueue& operator=(const SharedQueue&) = delete;  // disable assignment

};

#endif

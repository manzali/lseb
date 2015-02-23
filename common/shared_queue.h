#ifndef COMMON_SHARED_QUEUE_H
#define COMMON_SHARED_QUEUE_H

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
  T pop() {
    std::unique_lock<std::mutex> mlock(mutex_);
    while (queue_.empty()) {
      cond_.wait(mlock);
    }
    T item(queue_.front());
    queue_.pop();
    return item;
  }

  void push(const T& item) {
    std::unique_lock<std::mutex> mlock(mutex_);
    queue_.push(item);
    cond_.notify_one();
  }

  bool empty() {
    std::unique_lock<std::mutex> mlock(mutex_);
    return queue_.empty();
  }

  SharedQueue() = default;
  SharedQueue(const SharedQueue&) = delete;            // disable copying
  SharedQueue& operator=(const SharedQueue&) = delete;  // disable assignment

};

#endif

#ifndef COMMON_HANDLEREXECUTOR_HPP
#define COMMON_HANDLEREXECUTOR_HPP

#include <thread>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <deque>
#include <algorithm>

class HandlerExecutor {
 public:
  HandlerExecutor(size_t threads)
      :
        stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
      busy_.emplace_back(0);
      workers_.emplace_back([&, i]() {
        while (!stop_) {
          std::unique_lock<std::mutex> lock(mutex_);
          while (!stop_ && tasks_.empty()) {
            cond_var_.wait(lock);
          }
          if (!stop_) {
            std::function<void()> task = tasks_.front();
            busy_[i] = 1;
            tasks_.pop_front();
            lock.unlock();
            task();
            busy_[i] = 0;
          }
        }
      });
    }
  }

  ~HandlerExecutor() {
    stop_ = true;
    cond_var_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  template<typename HandlerType>
  void post(HandlerType handler) {
    std::unique_lock<std::mutex> lock(mutex_);
    tasks_.push_back(std::function<void()>(handler));
    lock.unlock();
    cond_var_.notify_one();
  }

  void wait() {
    while (!tasks_.empty() || std::any_of(
      std::begin(busy_),
      std::end(busy_),
      [](char i) {return i != 0;})) {
      std::mutex mutex;
      std::condition_variable cond_var;
      std::unique_lock<std::mutex> lock(mutex);
      post([&]() {
        cond_var.notify_one();
      });
      cond_var.wait(lock);
    }
  }

 private:
  std::vector<std::thread> workers_;
  std::vector<char> busy_;
  std::deque<std::function<void()> > tasks_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
  bool stop_;
};

#endif

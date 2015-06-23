#ifndef COMMON_HANDLEREXECUTOR_HPP
#define COMMON_HANDLEREXECUTOR_HPP

#include <thread>
#include <memory>
#include <vector>
#include <map>
#include <atomic>

#include <cassert>

#include <boost/asio.hpp>

class HandlerExecutor {
 public:
  HandlerExecutor(size_t n_threads)
      :
        work_(new boost::asio::io_service::work(io_service_)),
        tokens_(0) {
    for (size_t i = 0; i < n_threads; ++i) {
      threads_.emplace_back([&]() {io_service_.run();});
    }
  }

  ~HandlerExecutor() {
    work_.reset();
    for (auto& th : threads_) {
      th.join();
    }
  }

  void post(std::function<void()> (handler)) {
    ++tokens_;
    io_service_.post([&]() {
      handler();
      if(--tokens_ == 0) {
        cond_var_.notify_all();
      }
    });
  }

  void post(int sequence_id, std::function<void()> (handler)) {
    ++tokens_;
    auto seq_it = sequences_.insert(
      std::make_pair(sequence_id, std::move(boost::asio::strand(io_service_))));
    seq_it.first->second.post([&]() {
      handler();
      if(--tokens_ == 0) {
        cond_var_.notify_all();
      }
    });
  }

  void wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (tokens_ != 0) {
      cond_var_.wait(lock);
    }
  }

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  std::map<int, boost::asio::strand> sequences_;
  std::vector<std::thread> threads_;
  std::atomic<int> tokens_;
  std::mutex mutex_;
  std::condition_variable cond_var_;
};

#endif

#ifndef HANDLEREXECUTOR_HPP
#define HANDLEREXECUTOR_HPP

#include <thread>
#include <memory>
#include <vector>
#include <chrono>

#include <cassert>

#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

class HandlerExecutor {
 public:
  HandlerExecutor(size_t n_threads)
      :
        work_(new boost::asio::io_service::work(io_service_)),
        stop_(false) {
    for (size_t i = 0; i < n_threads; ++i) {
      threads_.emplace_back([&]() {io_service_.run();});
    }
  }

  ~HandlerExecutor() {
    assert(!work_.get() && "Call wait before destructor!");
  }

  void post(std::function<void()> handler) {
    if (!stop_) {
      io_service_.post(handler);
    }
  }

  void wait(std::chrono::high_resolution_clock::time_point end_time) {
    boost::asio::high_resolution_timer timer(io_service_);
    timer.expires_at(end_time);
    timer.async_wait([&](const boost::system::error_code&) {stop_ = true;});
    wait();
    stop_ = false;
  }

  void wait() {
    assert(work_.get() && "There is no work!");
    work_.reset();
    for (auto& th : threads_) {
      th.join();
    }
    io_service_.reset();
    size_t n_threads = threads_.size();
    threads_.clear();
    work_.reset(new boost::asio::io_service::work(io_service_));
    for (size_t i = 0; i < n_threads; ++i) {
      threads_.emplace_back([&]() {io_service_.run();});
    }
  }

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  std::vector<std::thread> threads_;

  bool stop_;
};

#endif

#ifndef HANDLEREXECUTOR_HPP
#define HANDLEREXECUTOR_HPP

#include <thread>
#include <memory>
#include <vector>
#include <map>

#include <cassert>

#include <boost/asio.hpp>

class HandlerExecutor {
 public:
  HandlerExecutor(size_t n_threads)
      :
        work_(new boost::asio::io_service::work(io_service_)) {
    for (size_t i = 0; i < n_threads; ++i) {
      threads_.emplace_back([&]() {io_service_.run();});
    }
  }

  ~HandlerExecutor() {
    assert(!work_.get() && "Call stop before destructor!");
  }

  template<typename HandlerType>
  void post(HandlerType handler) {
    io_service_.post(handler);
  }

  template<typename HandlerType>
  void post(int sequence_id, HandlerType handler) {
    auto seq_it = sequences_.insert(
      std::make_pair(sequence_id, std::move(boost::asio::strand(io_service_))));
    seq_it.first->second.post(handler);
  }

  void stop() {
    assert(work_.get() && "There is no work!");
    work_.reset();
    for (auto& th : threads_) {
      th.join();
    }
  }

 private:
  boost::asio::io_service io_service_;
  std::unique_ptr<boost::asio::io_service::work> work_;
  std::map<int, boost::asio::strand> sequences_;
  std::vector<std::thread> threads_;
};

#endif

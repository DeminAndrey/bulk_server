#pragma once

#include "async/async.h"

#include <iostream>
#include <memory>

#include <boost/asio/ip/tcp.hpp>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>

namespace detail {

template <class T>
struct task_wrapped {
private:
    T task_unwrapped_;

public:
    explicit task_wrapped(const T& f)
        : task_unwrapped_(f)
    {}

    void operator()() const {
        try {
            boost::this_thread::interruption_point();
        } catch(const boost::thread_interrupted&){}

        try {
            task_unwrapped_();
        } catch (const std::exception& e) {
            std::cerr<< "Exception: " << e.what() << '\n';
        } catch (const boost::thread_interrupted&) {
            std::cerr<< "Thread interrupted\n";
        } catch (...) {
            std::cerr<< "Unknown exception\n";
        }
    }
};

template <class T>
task_wrapped<T> make_task_wrapped(const T& task_unwrapped) {
    return task_wrapped<T>(task_unwrapped);
}

} // namespace detail

struct connection_with_data : boost::noncopyable {
  boost::asio::ip::tcp::socket socket;
  std::string data;
  async::handle_t handler;

  explicit connection_with_data(boost::asio::io_context &ioc)
    : socket(ioc) {}

  void shutdown() {
    boost::system::error_code ignore;
    async::disconnect(handler);
    socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignore);
    socket.close(ignore);
  }

  ~connection_with_data() {
    shutdown();
  }
};

template<typename T>
struct task_wrapped_with_connection {

private:
  std::unique_ptr<connection_with_data> connect_;
  T task_unwrapped_;

public:
  explicit task_wrapped_with_connection(
      std::unique_ptr<connection_with_data> &&connect, const T &f)
    : connect_(std::move(connect)),
      task_unwrapped_(f) {}

  void operator()(const boost::system::error_code &code, size_t bytes_count) {
    const auto lambda = [this, &code, bytes_count] {
      connect_->data.resize(bytes_count);
      task_unwrapped_(std::move(connect_), code);
    };

    const auto task = detail::make_task_wrapped(lambda);
    task();
  }
};

class task_processor {
  typedef boost::asio::ip::tcp::acceptor acceptor_t;
  typedef boost::function<void(std::unique_ptr<connection_with_data>,
                               const boost::system::error_code &)> on_accept_func_t;

  struct tcp_listener {
    unsigned bulk_size = 0;
    acceptor_t acceptor_;
    const on_accept_func_t func_;
    std::unique_ptr<connection_with_data> new_c_;

    template<typename Functor>
    tcp_listener(boost::asio::io_context &io_context,
                 unsigned short port, unsigned size,
                 const Functor &task_unwrapped)
      : bulk_size(size),
        acceptor_(io_context, boost::asio::ip::tcp::endpoint(
                    boost::asio::ip::tcp::v4(), port)),
        func_(task_unwrapped) {
    }
  };

  struct handle_accept {
    std::unique_ptr<tcp_listener> listener;

    explicit handle_accept(std::unique_ptr<tcp_listener> &&l)
      : listener(std::move(l)) {
    }

    void operator()(const boost::system::error_code &error) {
      task_wrapped_with_connection<on_accept_func_t> task(
            std::move(listener->new_c_), listener->func_);

      start_accepting_connection(std::move(listener));
      task(error, 0);
    }
  };

protected:
  static boost::asio::io_context &get_ioc() {
    static boost::asio::io_context ioc;
    static boost::asio::io_context::work work(ioc);

    return ioc;
  }

public:
  static void start() {
    get_ioc().run();
  }

  static void stop() {
    get_ioc().stop();
  }

  static std::unique_ptr<connection_with_data> create_connection(
      const char *addr, unsigned short port_num) {
    std::unique_ptr<connection_with_data> connect(
          new connection_with_data(get_ioc()));
    connect->socket.connect(
          boost::asio::ip::tcp::endpoint(
            boost::asio::ip::address_v4::from_string(addr), port_num));

    return connect;
  }

  template<typename Functor>
  static void add_listener(unsigned port_num,
                           unsigned bulk_size, const Functor &f) {
    std::unique_ptr<tcp_listener> listener(
          new tcp_listener(get_ioc(), port_num, bulk_size, f));

    start_accepting_connection(std::move(listener));
  }

private:
  static void start_accepting_connection(
      std::unique_ptr<tcp_listener> && listener) {
    if(!listener->acceptor_.is_open()) {
      return;
    }
    listener->new_c_.reset(new connection_with_data(get_ioc()));
    listener->new_c_->handler = async::connect(listener->bulk_size);

    boost::asio::ip::tcp::socket &s = listener->new_c_->socket;
    acceptor_t &a = listener->acceptor_;
    a.async_accept(s, task_processor::handle_accept(std::move(listener)));
  }
};

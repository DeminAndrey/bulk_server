#pragma once

#include "async/async.h"
#include "command_processor.h"
#include "task_processor.h"

#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>

template<typename Functor>
void async_write_data(std::unique_ptr<connection_with_data> &&c,
                      const Functor &f) {
  auto &d = c->data;
  boost::asio::async_write(
        c->socket, boost::asio::buffer(d, d.size()),
        task_wrapped_with_connection<Functor>(std::move(c), f));
}

template<typename Functor>
void async_read_data(std::unique_ptr<connection_with_data> &&c,
                     const Functor &f, size_t at_least_bytes) {
  c->data.resize(at_least_bytes);
  auto &d = c->data;
  char *p = d.empty() ? nullptr : &d.front();

  boost::asio::async_read(
        c->socket, boost::asio::buffer(p, d.size()),
        task_wrapped_with_connection<Functor>(std::move(c), f));
}

template <class Functor>
void async_read_data_at_least(std::unique_ptr<connection_with_data>&& c,
                              const Functor& f,
                              std::size_t at_least_bytes,
                              std::size_t at_most) {
  std::string& d = c->data;
  d.resize(at_most);
  char* p = (at_most == 0 ? 0 : &d[0]);

  boost::asio::ip::tcp::socket& s = c->socket;

  boost::asio::async_read(
        s,
        boost::asio::buffer(p, at_most),
        boost::asio::transfer_at_least(at_least_bytes),
        task_wrapped_with_connection<Functor>(std::move(c), f)
        );
}

void process_server_response(
    std::unique_ptr<connection_with_data> &&c,
    const boost::system::error_code &code) {

  if(code && code != boost::asio::error::eof) {
    std::cerr << "Client error on receive: " << code.message() << "\n";
    assert(false);
  }

  if(c->data.size() != 2) {
    std::cerr << "Wrong bytes count\n";
    assert(false);
  }

  if(c->data != "OK") {
    std::cerr << "Wrong response: " << c->data << "\n";
    assert(false);
  }

  c->shutdown();
  task_processor::stop();
}

void receive_data_response(std::unique_ptr<connection_with_data> &&c,
                           const boost::system::error_code &code) {
  if(code) {
    std::cerr << "Error on sending data: " << code.message() << "\n";
    assert(false);
  }

  async_read_data(std::move(c), &process_server_response, 2);
}

namespace session {
std::unique_ptr<connection_with_data> create_connection(unsigned port) {
  return task_processor::create_connection("127.0.0.1", port);
}

void send_data(std::unique_ptr<connection_with_data> &soc,
               const std::string &data) {
  soc->data = data;
  async_write_data(std::move(soc), &receive_data_response);
}
} // namespace session

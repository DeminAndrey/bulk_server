#pragma once

#include "async.h"
#include "command_processor.h"
#include "task_processor.h"
#include "task_sender.h"

#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>

class data_handler {
public:
  static void on_connection_accept(
      std::unique_ptr<connection_with_data> &&connection,
      const boost::system::error_code &error) {
    assert(!error);
    async_read_data_at_least(std::move(connection), &data_handler::on_datarecieve, 1, 1024);
  }

  static void on_datarecieve(
      std::unique_ptr<connection_with_data> &&connection,
      const boost::system::error_code& error) {
      if (error) {
          std::cerr << "handler_.on_datarecieve: error during recieving response: " << error << '\n';
          assert(false);
      }

      if (connection->data.size() == 0) {
          std::cerr << "handler_.on_datarecieve: zero bytes recieved\n";
          assert(false);
      }

      async::receive(connection->handler, connection->data.c_str(), connection->data.size());

      connection->data = "OK";
      async_write_data(std::move(connection), &data_handler::on_datasend);
  }

  static void on_datasend(std::unique_ptr<connection_with_data>&& connection,
                          const boost::system::error_code& error) {
      if (error) {
          std::cerr << "handler_.on_datasend: error during sending response: " << error << '\n';
          assert(false);
      }

      connection->shutdown();
  }
};

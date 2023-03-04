#include "task_sender.h"
#include "task_receiver.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Not enough parameters to run!" << std::endl;

    return 1;
  }
  unsigned port = 0;
  unsigned bulk_size = 0;
  try {
    port = std::atoi(argv[0]);
    bulk_size = std::atoi(argv[1]);
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what();

    return 1;
  }

  std::string buffer;
  std::cin >> buffer;
  auto size = buffer.length();

  auto connection = create_connection(port, bulk_size);
  send_data(connection, buffer);

  task_processor::add_listener(port, &data_handler::on_connection_accept);
  task_processor::start();

  disconnect(std::move(connection));

  return 0;
}

#include "server.h"

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

  task_processor::add_listener(
        port, bulk_size, &data_handler::on_connection_accept);
  task_processor::start();

  return 0;
}

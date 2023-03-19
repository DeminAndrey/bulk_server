#include "server.h"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Not enough parameters to run!" << std::endl;

    return 1;
  }  
  unsigned port = static_cast<unsigned>(std::atoi(argv[0]));
  unsigned bulk_size = static_cast<unsigned>(std::atoi(argv[1]));
  if (port == 0 || bulk_size == 0) {
    std::cerr << "Invalid argument\n";

    return 1;
  }

  task_processor::add_listener(
        port, bulk_size, &data_handler::on_connection_accept);
  task_processor::start();

  return 0;
}

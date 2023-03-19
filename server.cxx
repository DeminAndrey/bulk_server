#include "server.h"

#include <boost/program_options.hpp>

static const char *HELP = "help";
static const char *PORT = "port";
static const char *BULK_SIZE = "size";

namespace po = boost::program_options;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Not enough parameters to run!" << std::endl;

    return 1;
  }  
  po::options_description opt_desc("Allowed options");
  opt_desc.add_options()
      (HELP,                                          "Print this message")
      (PORT, po::value<unsigned>()->required(),       "Server port")
      (BULK_SIZE, po::value<unsigned>()->required(),  "Bulk size");

  po::variables_map var_map;
  try {
    auto parsed = po::command_line_parser(argc, argv)
        .options(opt_desc)
        .run();
    po::store(parsed, var_map);
    if (var_map.count(HELP)) {
      std::cout << opt_desc << "\n";

      return 0;
    }
    po::notify(var_map);
  }
  catch (const po::error &err) {
    std::cerr << "Error while parsing command-line arguments: "
              << err.what() << "\nPlease use --help to see help message\n";
    return 1;
  }

  auto port = var_map[PORT].as<unsigned>();
  auto bulk_size = var_map[BULK_SIZE].as<unsigned>();

  task_processor::add_listener(
        port, bulk_size, &data_handler::on_connection_accept);
  task_processor::start();

  return 0;
}

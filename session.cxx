#include "session.h"

#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

static const char *HELP = "help";
static const char *PORT = "port";
static const char *SEQUENCE = "sequence";
static const char *RECEIVE = "receive";

namespace po = boost::program_options;

namespace {
std::vector<std::string> parse_sequence(const std::string &seq) {
  std::string str = seq;
  boost::trim(str);
  if (str.empty()) {
    throw std::invalid_argument("Empty sequence");
  }
  std::vector<std::string> result;
  boost::algorithm::split(result, str, boost::is_any_of(" "),
                          boost::token_compress_on);
  if (result.size() != 2) {
    throw std::invalid_argument("Unknown sequence");
  }
  const auto start = std::atoi(result.front().c_str());
  const auto end = std::atoi(result.back().c_str());
  if (start == 0 && end == 0) {
    throw std::invalid_argument("Zero expression");
  }
  if (start < 0 || end < 0) {
    throw std::invalid_argument("Negative expression");
  }
  if (start > end) {
    throw std::range_error("The second argument is greater than the first");
  }

  std::vector<std::string> commands;
  for (int i = start; i < end; ++i) {
    commands.push_back(std::to_string(i));
  }

  return commands;
}
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Not enough parameters to run!" << std::endl;

    return 1;
  }
  unsigned port = 0;
  std::string buffer;

  po::options_description opt_desc("Allowed options");
  opt_desc.add_options()
      (HELP,                                "Print this message")
      (SEQUENCE, po::value<std::string>(),  "Sequence from ... and to ...")
      (RECEIVE, po::value<std::string>(),   "Expression to send to the server")
      (PORT, po::value<std::size_t>()->required(), "Server port");

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
    if (!var_map.count(SEQUENCE) ||
        !var_map.count(RECEIVE)) {
      std::cerr << "Not enough parameters to send to server\n"
                << "Please use --help to see help message\n";

      return 1;
    }
    if (var_map.count(SEQUENCE) &&
        var_map.count(RECEIVE)) {
      std::cerr << "Too many parameters\n"
                << "Please use --help to see help message\n";

      return 1;
    }
    po::notify(var_map);
  }
  catch (const po::error &err) {
    std::cerr << "Error while parsing command-line arguments: "
              << err.what() << "\nPlease use --help to see help message\n";
    return 1;
  }

  port = var_map[PORT].as<unsigned>();
  if (var_map.count(RECEIVE)) {
    auto buffer = var_map[RECEIVE].as<std::string>();
    if (!buffer.empty()) {
      auto connection = session::create_connection(port);
      session::send_data(connection, buffer);
    }
  }
  else {
    try {
      auto connection = session::create_connection(port);
      for (const auto &buffer : parse_sequence(
             var_map[SEQUENCE].as<std::string>())) {
        session::send_data(connection, buffer);
      }
    }
    catch (const std::exception &ex) {
      std::cerr << "Error while parsing sequence: " << ex.what();

      return 1;
    }
  }

  return 0;
}

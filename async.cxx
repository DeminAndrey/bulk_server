#include "async.h"

#include "command_processor.h"

#include <boost/algorithm/string.hpp>

namespace async {

handle_t connect(size_t bulk) {
  return new batch_console_input(bulk);
}

void receive(handle_t handle, const char *data, size_t) {
  std::vector<std::string> parts;
  boost::algorithm::split(parts, data, boost::algorithm::is_any_of("\n"),
                          boost::token_compress_on);
  parts.erase(std::remove(parts.begin(), parts.end(), ""), parts.end());

  if (auto h = static_cast<batch_console_input*>(handle)) {
    for (const auto &part : parts) {
      h->process_command(command{part, std::chrono::system_clock::now()});
    }
  }
}

void disconnect(handle_t handle) {
  if (auto h = static_cast<batch_console_input*>(handle)) {
    delete h;
  }
}
} // namespace async

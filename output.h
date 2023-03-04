#pragma once

#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static const std::string BULK = "bulk: ";

struct command {
  std::string text;
  std::chrono::system_clock::time_point time_stamp;
};

struct block_command {
  std::chrono::system_clock::time_point block_time;
  std::vector<command> commands;
};

std::string join(const std::vector<command>& v) {
  return std::accumulate(v.begin(), v.end(), std::string(),
                         [](std::string &s, const command &com) {
    return s.empty() ? s.append(com.text)
                     : s.append(", ").append(com.text);
  });
}

/**
 * @brief класс вывода команд в консоль
 */
class console_output {

public:
  console_output() = default;

  static void push_block(const block_command &block) {
    m_context.push_command(block);
  }

  static void print() {
    std::thread log(print_block);
    log.join();
  }

private:
  console_output(const console_output &) = delete;
  console_output &operator=(const console_output &) = delete;

  static void print_block() {
    while (!m_context.is_empty()) {
      auto block = m_context.try_pop_command();
      auto output = BULK + join(block.commands);
      std::cout << output << std::endl;
    }
  }
};


/**
 * @brief класс записи команд в файл
 */
class file_writer {

public:
  file_writer() = default;

  static void push_block(const block_command &block) {
    m_context.push_command(block);
  }

  static void write() {
    while (!m_context.is_empty()) {
      auto block = m_context.try_pop_command();
      write_block(block);
    }
  }

  static void async_write() {
    std::thread file_1(write);
    std::thread file_2(write);

    file_1.join();
    file_2.join();
  }

private:
  static command_queue m_context;

  file_writer(const file_writer &) = delete;
  file_writer &operator=(const file_writer &) = delete;

  static void write_block(const block_command &block) {
    auto output = BULK + join(block.commands);
    std::ofstream file(get_filename(block), std::ofstream::out);
    file << output;
  }

  static std::string get_filename(const block_command &block) {
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(
          block.block_time.time_since_epoch()).count();
    std::stringstream filename;
    auto id = std::this_thread::get_id();
    filename << "bulk" << seconds << "_" << id << ".log";

    return filename.str();
  }
};

command_queue console_output::m_context;
command_queue file_writer::m_context;

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

static std::string join(const std::vector<command>& v) {
  return std::accumulate(v.begin(), v.end(), std::string(),
                         [](std::string &s, const command &com) {
    return s.empty() ? s.append(com.text)
                     : s.append(", ").append(com.text);
  });
}

class command_queue {
  std::deque<block_command> m_blocks;
  std::mutex m_mutex;
  std::condition_variable m_condition;

public:

  void push_command(const block_command &block) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_blocks.push_back(block);
    lock.unlock();

    m_condition.notify_one();
  }

  block_command pop_command() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_blocks.empty()) {
      m_condition.wait(lock);
    }

    block_command ret = m_blocks.front();
    m_blocks.pop_front();

    return ret;
  }

  bool is_empty() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocks.empty();
  }
};


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
  static command_queue m_context;

  console_output(const console_output &) = delete;
  console_output &operator=(const console_output &) = delete;

  static void print_block() {
    while (!m_context.is_empty()) {
      auto block = m_context.pop_command();
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
      auto block = m_context.pop_command();
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

inline command_queue console_output::m_context;
inline command_queue file_writer::m_context;

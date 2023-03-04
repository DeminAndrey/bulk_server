#pragma once

#include "output.h"

#include <memory>

static const std::string START_BLOCK = "{";
static const std::string END_BLOCK = "}";

/**
 * @brief класс обработчика команд
 */
class batch_command_processor {
  size_t m_bulk_size;
  bool m_block_forced = false;
  std::vector<command> m_commands;

public:
  explicit batch_command_processor(size_t bulk_size)
    : m_bulk_size(bulk_size) {
  }

  ~batch_command_processor() {
    if (!m_block_forced) {
      dump_batch();
    }
  }

  void start_block() {
    m_block_forced = true;
    dump_batch();
  }

  void finish_block() {
    m_block_forced = false;
    dump_batch();
  }

  void process_command(const command& command) {
    m_commands.push_back(command);
    if (!m_block_forced &&
        (m_commands.size() >= static_cast<size_t>(m_bulk_size))) {
      dump_batch();
    }
  }

private:
  void clear_batch() {
    m_commands.clear();
  }

  void dump_batch() {
    if (!m_commands.empty()) {
      block_command block{m_commands.front().time_stamp, m_commands};
      console_output::push_block(block);
      file_writer::push_block(block);
    }
    console_output::print();
    file_writer::async_write();
    clear_batch();
  }
};

/**
 * @brief класс работы с командами из консоли
 */
class batch_console_input {
  int m_block_depth = 0;
  std::unique_ptr<batch_command_processor> m_command_processor;

public:
  explicit batch_console_input(size_t bulk_size) {
    m_command_processor = std::make_unique<batch_command_processor>(bulk_size);
  }

  void process_command(const command& command) {
    if (m_command_processor) {
      if (command.text == START_BLOCK) {
        if (m_block_depth++ == 0)
          m_command_processor->start_block();
      }
      else if (command.text == END_BLOCK) {
        if (--m_block_depth == 0)
          m_command_processor->finish_block();
      }
      else
        m_command_processor->process_command(command);
    }
  }
};

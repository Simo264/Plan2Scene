#pragma once

#include "types.hpp"

#include <string>
#include <vector>
#include <mutex>

enum class LogLevel : i32
{
  Text = 0xFFFFFF,
  Success = 0x80FF00,
  Info = 0x0080FF,
  Warning = 0xFF8000,
  Error = 0xFF3333,
};

struct LogMessage
{
  std::string message;
  LogLevel level;
};

class Logger
{
  public:
    Logger() = default;
    ~Logger() = default;

    void push_message(LogMessage msg)
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_messages.push_back(std::move(msg));
    }

    auto get_messages() const
    {
      std::lock_guard<std::mutex> lock(m_mutex);
      return m_messages;
    }
    
  private:
    mutable std::mutex m_mutex;
    std::vector<LogMessage> m_messages;
};
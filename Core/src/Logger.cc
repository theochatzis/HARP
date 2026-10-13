#include "Core/interface/Logger.h"

#include <iostream>
#include <mutex>

namespace harp {

namespace {
LogLevel& globalLevel() {
  static LogLevel level = LogLevel::Info;
  return level;
}
std::mutex& logMutex() {
  static std::mutex m;
  return m;
}
const char* tag(LogLevel l) {
  switch (l) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO ";
    case LogLevel::Warning: return "WARN ";
    case LogLevel::Error: return "ERROR";
  }
  return "?";
}
}  // namespace

void Logger::setLevel(LogLevel level) { globalLevel() = level; }
LogLevel Logger::level() { return globalLevel(); }

LogLevel Logger::levelFromString(const std::string& s) {
  if (s == "DEBUG" || s == "debug") return LogLevel::Debug;
  if (s == "WARNING" || s == "warning" || s == "WARN") return LogLevel::Warning;
  if (s == "ERROR" || s == "error") return LogLevel::Error;
  return LogLevel::Info;
}

void Logger::log(LogLevel level, const std::string& context, const std::string& message) {
  if (level < globalLevel()) return;
  std::lock_guard<std::mutex> lock(logMutex());
  std::ostream& os = (level >= LogLevel::Warning) ? std::cerr : std::cout;
  os << "[" << tag(level) << "] " << context << ": " << message << std::endl;
}

void Logger::banner(const std::string& title) {
  std::lock_guard<std::mutex> lock(logMutex());
  std::cout << "\n==================== " << title << " ====================\n" << std::endl;
}

}  // namespace harp

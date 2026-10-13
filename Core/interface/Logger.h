#ifndef HARP_CORE_LOGGER_H
#define HARP_CORE_LOGGER_H
// Minimal stream logger. Usage:
//   HARP_INFO("MyModule") << "processed " << n << " events";
//   Logger::setLevel(LogLevel::Debug);
#include <sstream>
#include <string>

namespace harp {

enum class LogLevel { Debug = 0, Info = 1, Warning = 2, Error = 3 };

class Logger {
 public:
  static void setLevel(LogLevel level);
  static LogLevel level();
  static LogLevel levelFromString(const std::string& s);
  static void log(LogLevel level, const std::string& context, const std::string& message);
  static void banner(const std::string& title);
};

// Temporary stream that emits one line on destruction.
class LogStream {
 public:
  LogStream(LogLevel level, std::string context) : level_(level), context_(std::move(context)) {}
  ~LogStream() { Logger::log(level_, context_, stream_.str()); }
  template <class T>
  LogStream& operator<<(const T& v) {
    stream_ << v;
    return *this;
  }

 private:
  LogLevel level_;
  std::string context_;
  std::ostringstream stream_;
};

}  // namespace harp

#define HARP_DEBUG(ctx) ::harp::LogStream(::harp::LogLevel::Debug, ctx)
#define HARP_INFO(ctx) ::harp::LogStream(::harp::LogLevel::Info, ctx)
#define HARP_WARNING(ctx) ::harp::LogStream(::harp::LogLevel::Warning, ctx)
#define HARP_ERROR(ctx) ::harp::LogStream(::harp::LogLevel::Error, ctx)

#endif

#ifndef HUE__LOGGER_H
#define HUE__LOGGER_H

// Use these macros in your code (Don't use hue::Logger::begin etc).
//
// Example:
//
//   // Debug message
//   rlog("Response code: " << rsp.code << " (" << rsp.toString());
//
//   // Warning message
//   rlogw("Failed to chmod(\"" << filename << "\", " << mode << ")");
//
//   // With explicit level
//   rlogl(hue::Logger::Warning, "Failed to chmod(\"" << filename << "\", "
//          << mode << ")");
//
#if NDEBUG
  // Strip trace and debug messages from non-debug builds
  #define rtrace() do{}while(0)
  #define rlog(...) do{}while(0)
#else
  #define rtrace() rlogl(hue::Logger::Trace, "")
  #define rlog(args) rlogl(hue::Logger::Debug, args)
#endif
#define rlogw(args) rlogl(hue::Logger::Warning, args)
#define rloge(args) rlogl(hue::Logger::Error, args)

#define rlogl(level, A) do { if (level >= hue::Logger::currentLevel) { \
  hue::Logger::end(hue::Logger::begin(level) << A, __FILE__, __LINE__); } } while(0)

// ----------------------------------------------------------------------------

#include <unistd.h>
#include <stdint.h>
#include <iostream>

namespace hue {

class Logger {
public:
  enum Level {
    Trace = 0,
    Debug,
    Warning,
    Error,
  };
  static Level currentLevel;
  static volatile uint8_t isATTY;
  static std::ostream& begin(const Logger::Level level) {
    if (Logger::isATTY == 0) Logger::isATTY = isatty(1) ? 1 : 2;
    std::ostream& s = std::clog;
    if (Logger::isATTY) {
      s << (level == Trace ? "\e[30;47m "
          : level == Debug ? "\e[30;44m "
          : level == Warning ? "\e[30;43m "
          : "\e[30;41m ");
    }
    return s << (level == Trace ? 'T'
               : level == Debug ? 'D'
               : level == Warning ? 'W'
               : 'E') << (Logger::isATTY ? " \e[0m " : " ");
  }
  static std::ostream& end(std::ostream& s, const char *filename, const int lineno) {
    s << (Logger::isATTY ? " \e[30;1;40m[" : " [")
      << filename << ':' << lineno
      << (Logger::isATTY ? "]\e[0m" : "]");
    return s << std::endl;// << std::flush;
  }
private:
  explicit Logger() {}
};

} // namespace hue
#endif // HUE__LOGGER_H

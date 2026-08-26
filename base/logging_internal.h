#ifndef BASE_LOGGING_INTERNAL_H_
#define BASE_LOGGING_INTERNAL_H_

#include <sys/types.h>

#include <string>
#include <variant>

#include "base/time.h"

namespace base::internal {

enum class LogSeverity {
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

// Fixed-layout record for a single log message.
// Uses a variant payload to bypass string allocations for compile-time
// literals.
struct LogEntry final {
  LogSeverity severity = LogSeverity::kInfo;
  base::WallTime timestamp;
  pid_t tid = 0;
  const char* file = nullptr;
  int line = 0;

  // Payload: const char* points directly to static/literal strings (zero
  // allocation), std::string handles dynamic, formatted text on the heap when
  // needed.
  std::variant<const char*, std::string> message;

  // Formats the entry into a standard human-readable string:
  // "SYYYYMMDD HH:MM:SS.uuuuuu <tid> <file>:<line>] <message>\n"
  [[nodiscard]] std::string ToString() const;
};

}  // namespace base::internal

#endif  // #ifndef BASE_LOGGING_INTERNAL_H_

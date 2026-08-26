#ifndef BASE_LOGGING_INTERNAL_H_
#define BASE_LOGGING_INTERNAL_H_

namespace base::internal {

enum class LogSeverity {
  kInfo = 0,
  kWarning = 1,
  kError = 2,
  kFatal = 3,
};

struct LogEntry {
  LogSeverity severity = LogSeverity::kInfo;
  // payload fields...
};

}  // namespace base::internal

#endif  // #ifndef BASE_LOGGING_INTERNAL_H_

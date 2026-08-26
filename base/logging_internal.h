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

#define INFO (::base::internal::LogSeverity::kInfo)
#define WARNING (::base::internal::LogSeverity::kWarning)
#define ERROR (::base::internal::LogSeverity::kError)
#define FATAL (::base::internal::LogSeverity::kFatal)

#endif  // #ifndef BASE_LOGGING_INTERNAL_H_

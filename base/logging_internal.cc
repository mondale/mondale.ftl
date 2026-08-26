#include <sys/types.h>

#include <cstdio>
#include <ctime>
#include <string>
#include <variant>

#include "base/logging_internal.h"

namespace base::internal {

std::string LogEntry::ToString() const {
  char severity_char = 'I';
  switch (severity) {
    case LogSeverity::kInfo:
      severity_char = 'I';
      break;
    case LogSeverity::kWarning:
      severity_char = 'W';
      break;
    case LogSeverity::kError:
      severity_char = 'E';
      break;
    case LogSeverity::kFatal:
      severity_char = 'F';
      break;
  }

  const int64_t nanos = timestamp.UnixNanoseconds();
  const time_t seconds = static_cast<time_t>(nanos / 1000000000LL);
  // 5 digits of precision = 10-microsecond resolution (nanos / 10,000)
  const int32_t sub_second_10us =
      static_cast<int32_t>((nanos % 1000000000LL) / 10000LL);

  struct tm tm_time{};
  // TIMEZONE CHOICE: Using localtime_r for local timezone formatting.
  // To switch to UTC/GMT in the future, replace `localtime_r` with `gmtime_r`.
  localtime_r(&seconds, &tm_time);

  // Determine payload length to pre-size result buffer
  size_t msg_len = std::visit(
      [](auto&& arg) -> size_t {
        if constexpr (std::is_same_v<std::decay_t<decltype(arg)>,
                                     const char*>) {
          return arg ? std::char_traits<char>::length(arg) : 0;
        } else {
          return arg.size();
        }
      },
      message);

  // Header format: "S<tid> HH:MM:SS.uuuuu <file>:<line>] " (~60-80 bytes)
  constexpr size_t kEstimatedHeaderLen = 80;
  std::string result;
  result.reserve(kEstimatedHeaderLen + msg_len + 1);

  size_t header_capacity = kEstimatedHeaderLen;
  result.resize(header_capacity);

  int written = std::snprintf(
      result.data(), header_capacity + 1, "%c%d %02d:%02d:%02d.%05d %s:%d] ",
      severity_char, static_cast<int>(tid), tm_time.tm_hour, tm_time.tm_min,
      tm_time.tm_sec, sub_second_10us, file ? file : "unknown", line);

  if (written > static_cast<int>(header_capacity)) {
    header_capacity = static_cast<size_t>(written);
    result.resize(header_capacity);
    std::snprintf(
        result.data(), header_capacity + 1, "%c%d %02d:%02d:%02d.%05d %s:%d] ",
        severity_char, static_cast<int>(tid), tm_time.tm_hour, tm_time.tm_min,
        tm_time.tm_sec, sub_second_10us, file ? file : "unknown", line);
  } else if (written >= 0) {
    result.resize(static_cast<size_t>(written));
  }

  // Append payload directly to pre-allocated string
  std::visit([&result](auto&& arg) { result.append(arg); }, message);

  result.push_back('\n');
  return result;
}

}  // namespace base::internal

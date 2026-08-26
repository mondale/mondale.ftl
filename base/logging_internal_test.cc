#include <ctime>
#include <string>

#include "base/logging_internal.h"
#include "testing/testing.h"

namespace base::internal {
namespace {

base::WallTime MakeTestTime(int year, int month, int day, int hour, int min,
                            int sec, int64_t sub_second_nanos) {
  struct tm tm_time{};
  tm_time.tm_year = year - 1900;
  tm_time.tm_mon = month - 1;
  tm_time.tm_mday = day;
  tm_time.tm_hour = hour;
  tm_time.tm_min = min;
  tm_time.tm_sec = sec;
  tm_time.tm_isdst = -1;

  time_t epoch_sec = mktime(&tm_time);
  int64_t total_nanos =
      (static_cast<int64_t>(epoch_sec) * 1000000000LL) + sub_second_nanos;
  return base::WallTime::FromUnixNanoseconds(total_nanos);
}

TEST(LogEntryFormatsStringLiteralPayloadCorrectly) {
  LogEntry entry;
  entry.severity = LogSeverity::kError;
  entry.timestamp =
      MakeTestTime(2026, 8, 25, 10, 2, 14, 313510000LL);  // .31351 sec
  entry.tid = 90;
  entry.file = "foo.cc";
  entry.line = 71;
  entry.message = "Nope!";

  std::string formatted = entry.ToString();

  // Exactly matches: E90 10:02:14.31351 foo.cc:71] Nope!\n
  EXPECT_EQ(formatted, "E90 10:02:14.31351 foo.cc:71] Nope!\n");
}

TEST(LogEntryFormatsDynamicStringPayloadCorrectly) {
  LogEntry entry;
  entry.severity = LogSeverity::kInfo;
  entry.timestamp =
      MakeTestTime(2026, 8, 25, 9, 5, 1, 987654321LL);  // .98765 sec
  entry.tid = 1234;
  entry.file = "main.cc";
  entry.line = 100;
  entry.message = std::string("Dynamic error count: ") + std::to_string(404);

  std::string formatted = entry.ToString();

  EXPECT_EQ(formatted,
            "I1234 09:05:01.98765 main.cc:100] Dynamic error count: 404\n");
}

TEST(LogEntryFormatsSubSecondZeroPadding) {
  LogEntry entry;
  entry.severity = LogSeverity::kWarning;
  entry.timestamp =
      MakeTestTime(2026, 8, 25, 10, 0, 0, 50000LL);  // 5 10us units -> .00005
  entry.tid = 42;
  entry.file = "test.cc";
  entry.line = 1;
  entry.message = "Padding test";

  std::string formatted = entry.ToString();

  EXPECT_EQ(formatted, "W42 10:00:00.00005 test.cc:1] Padding test\n");
}

}  // namespace
}  // namespace base::internal

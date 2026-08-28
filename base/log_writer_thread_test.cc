#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#include "base/log_queue.h"
#include "base/log_writer_thread.h"
#include "base/logging_internal.h"
#include "base/sleep.h"
#include "base/thread.h"
#include "base/time.h"
#include "core/file.h"
#include "testing/testing.h"

namespace base::internal {
namespace {

TEST(LogWriterThreadTest_WritesAndRoutesLogEntries) {
  const auto path = GetLogPath();

  // Construct and push test entries
  LogEntry info_entry;
  info_entry.severity = LogSeverity::kInfo;
  info_entry.timestamp =
      base::WallTime::FromUnixNanoseconds(1756120134000000000LL);
  info_entry.tid = 42;
  info_entry.file = "writer_test.cc";
  info_entry.line = 105;
  info_entry.message = "Hello, asynchronous world!";

  LogEntry error_entry;
  error_entry.severity = LogSeverity::kError;
  error_entry.timestamp =
      base::WallTime::FromUnixNanoseconds(1756120134000000001LL);
  error_entry.tid = 42;
  error_entry.file = "writer_test.cc";
  error_entry.line = 110;
  error_entry.message = "Critical failure detected!";

  ASSERT_FALSE(LogWriterThread::Instance() == nullptr);
  auto* const q = LogWriterThread::Instance()->QueueForCpu(0);
  ASSERT_FALSE(nullptr == q);

  q->Push(info_entry);
  q->Push(error_entry);

  while (true) {
    // Read back contents and look for correct routing and formatting
    auto content = core::ReadContentsFromFile(path);

    ASSERT_TRUE(content.ok());

    if (content.ValueOrDie().empty()) {
      base::SleepFor(base::Milliseconds(20));
      continue;
    }

    EXPECT_THAT(content.ValueOrDie(),
                testing::HasSubstr(info_entry.ToString()));
    EXPECT_THAT(content.ValueOrDie(),
                testing::HasSubstr(error_entry.ToString()));
    break;
  }
}

}  // namespace
}  // namespace base::internal

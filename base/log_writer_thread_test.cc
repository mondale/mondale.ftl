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

class ScopedTempFile {
 public:
  ScopedTempFile() {
    char filename_template[] = "/tmp/log_writer_test_XXXXXX";
    fd_ = mkstemp(filename_template);
    EXPECT_GE(fd_, 0);
    filename_ = filename_template;
  }

  ~ScopedTempFile() {
    if (fd_ >= 0) {
      close(fd_);
    }
    ::unlink(filename_.c_str());
  }

  int fd() const { return fd_; }
  const std::string& filename() const { return filename_; }

 private:
  int fd_ = -1;
  std::string filename_;
};

TEST(LogWriterThreadTest_WritesAndRoutesLogEntries) {
  ScopedTempFile info_file;
  ScopedTempFile error_file;

  auto queue = std::make_unique<LogQueue>([]() {});
  LogQueue* raw_queue = queue.get();
  std::vector<std::unique_ptr<LogQueue>> queues;
  queues.push_back(std::move(queue));

  // Sink 1: Accepts Info and Warning (bitmask bits 0 and 1 -> 0x03)
  SinkState info_sink;
  info_sink.fd = info_file.fd();
  info_sink.severity_mask = (1 << static_cast<int>(LogSeverity::kInfo)) |
                            (1 << static_cast<int>(LogSeverity::kWarning));
  info_sink.last_reported_drops.resize(1, {0, 0, 0, 0});

  // Sink 2: Accepts Error and Fatal (bitmask bits 2 and 3 -> 0x0C)
  SinkState error_sink;
  error_sink.fd = error_file.fd();
  error_sink.severity_mask = (1 << static_cast<int>(LogSeverity::kError)) |
                             (1 << static_cast<int>(LogSeverity::kFatal));
  error_sink.last_reported_drops.resize(1, {0, 0, 0, 0});

  std::vector<SinkState> sinks;
  sinks.push_back(std::move(info_sink));
  sinks.push_back(std::move(error_sink));

  // Initialize the singleton thread once
  LogWriterThread::Init(std::move(queues), std::move(sinks));

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

  raw_queue->Push(info_entry);
  raw_queue->Push(error_entry);

  while (true) {
    // Read back contents and look for correct routing and formatting
    auto info_content = core::ReadContentsFromFile(info_file.filename());
    auto error_content = core::ReadContentsFromFile(error_file.filename());

    ASSERT_TRUE(info_content.ok());
    ASSERT_TRUE(error_content.ok());

    if (info_content.ValueOrDie().empty() || error_content.ValueOrDie().empty())
      continue;
    base::SleepFor(base::Milliseconds(20));
    EXPECT_EQ(info_content.ValueOrDie(), info_entry.ToString());
    EXPECT_EQ(error_content.ValueOrDie(), error_entry.ToString());
    break;
  }
}

}  // namespace
}  // namespace base::internal

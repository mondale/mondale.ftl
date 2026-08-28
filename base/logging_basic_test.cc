#include "base/logging.h"
#include "base/sleep.h"
#include "base/time.h"
#include "core/file.h"
#include "testing/testing.h"

using ::testing::HasSubstr;

namespace {

std::string WaitForLogContent(const std::string& expected_substring,
                              base::Duration timeout = base::Seconds(20)) {
  auto start = base::MonotonicTime::Now();
  while (base::MonotonicTime::Now() - start < timeout) {
    std::string contents =
        core::ReadContentsFromFile(base::internal::GetLogPath()).ValueOrDie();
    if (contents.find(expected_substring) != std::string::npos) {
      return contents;
    }
    base::SleepFor(base::Milliseconds(50));
  }
  return core::ReadContentsFromFile(base::internal::GetLogPath()).ValueOrDie();
}

}  // namespace

TEST(BasicLogOutput) {
  Log(INFO) << "Test info message 12345";

  std::string logs = WaitForLogContent("Test info message 12345");
  EXPECT_THAT(logs, HasSubstr("Test info message 12345"));
}

TEST(MultipleSeverities) {
  Log(WARNING) << "Warning message 67890";
  Log(ERROR) << "Error message ABCDE";

  std::string logs = WaitForLogContent("Warning message 67890");
  EXPECT_THAT(logs, HasSubstr("Warning message 67890"));

  logs = WaitForLogContent("Error message ABCDE");
  EXPECT_THAT(logs, HasSubstr("Error message ABCDE"));
}

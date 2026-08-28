#include "base/logging.h"
#include "base/sleep.h"
#include "base/time.h"
#include "core/file.h"
#include "testing/testing.h"

using ::testing::HasSubstr;
using ::testing::Not;

namespace {

std::string WaitForLogContent(const std::string& expected_substring,
                              base::Duration timeout = base::Seconds(2)) {
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

TEST(LogModifierFirst) {
  for (int i = 0; i < 5; ++i) {
    Log(INFO, First(3)) << "First test iteration " << i;
  }

  std::string logs = WaitForLogContent("First test iteration 2");
  EXPECT_THAT(logs, HasSubstr("First test iteration 0"));
  EXPECT_THAT(logs, HasSubstr("First test iteration 1"));
  EXPECT_THAT(logs, HasSubstr("First test iteration 2"));
  EXPECT_THAT(logs, Not(HasSubstr("First test iteration 3")));
  EXPECT_THAT(logs, Not(HasSubstr("First test iteration 4")));
}

TEST(LogModifierEvery) {
  for (int i = 0; i < 6; ++i) {
    Log(INFO, Every(3)) << "Every test iteration " << i;
  }

  std::string logs = WaitForLogContent("Every test iteration 3");
  EXPECT_THAT(logs, HasSubstr("Every test iteration 0"));
  EXPECT_THAT(logs, Not(HasSubstr("Every test iteration 1")));
  EXPECT_THAT(logs, Not(HasSubstr("Every test iteration 2")));
  EXPECT_THAT(logs, HasSubstr("Every test iteration 3"));
  EXPECT_THAT(logs, Not(HasSubstr("Every test iteration 4")));
  EXPECT_THAT(logs, Not(HasSubstr("Every test iteration 5")));
}

TEST(LogModifierIf) {
  Log(INFO, If(true)) << "If test true branch";
  Log(INFO, If(false)) << "If test false branch";
  Log(FATAL, If(false)) << "Still alive!";

  std::string logs = WaitForLogContent("If test true branch");
  EXPECT_THAT(logs, HasSubstr("If test true branch"));
  EXPECT_THAT(logs, Not(HasSubstr("If test false branch")));
}

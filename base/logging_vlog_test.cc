#include <atomic>

#include "base/logging.h"
#include "base/sleep.h"
#include "base/time.h"
#include "core/file.h"
#include "testing/testing.h"

using ::base::SetVlogLevel;
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

TEST(VlogBasicTest) {
  SetVlogLevel(2);

  VLOG(1) << "Vlog level 1 message";
  VLOG(2) << "Vlog level 2 message";
  VLOG(3) << "Vlog level 3 message should not appear";

  std::string logs = WaitForLogContent("Vlog level 2 message");
  EXPECT_THAT(logs, HasSubstr("Vlog level 1 message"));
  EXPECT_THAT(logs, HasSubstr("Vlog level 2 message"));
  EXPECT_THAT(logs, Not(HasSubstr("Vlog level 3 message should not appear")));

  // Reset verbosity
  SetVlogLevel(0);
}

TEST(DVlogBasicTest) {
  SetVlogLevel(1);

  DVLOG(1) << "DVlog level 1 message";

#ifdef NDEBUG
  // In release builds, DVLOG is compiled out entirely.
  // We can just verify it doesn't show up.
  base::SleepFor(base::Milliseconds(10));
  std::string logs =
      core::ReadContentsFromFile(base::internal::GetLogPath()).ValueOrDie();
  EXPECT_THAT(logs, Not(HasSubstr("DVlog level 1 message")));
#else
  // In debug builds, DVLOG acts like VLOG.
  std::string logs = WaitForLogContent("DVlog level 1 message");
  EXPECT_THAT(logs, HasSubstr("DVlog level 1 message"));
#endif

  SetVlogLevel(0);
}

TEST(VmoduleTest) {
  base::SetVmodules("logging_vlog_test=1,foo=3,meh=414");
  VLOG(2) << "Oh hell no";
  VLOG(1) << "One msg";
  std::string logs = WaitForLogContent("One msg");
  EXPECT_THAT(logs, Not(HasSubstr("Oh hell no")));
}

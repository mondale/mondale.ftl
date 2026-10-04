#include "base/process.h"
#include "base/sleep.h"
#include "testing/testing.h"

namespace {

TEST(BasicProcessTest_StartTime) {
  EXPECT_NE(base::WallTime(), base::ProcessStartTime());
}

TEST(BasicProcessTest_UptimeAscending) {
  const auto up = base::ProcessUptime();
  base::SleepFor(base::Milliseconds(1));
  EXPECT_GT(base::ProcessUptime(), up);
}

}  // namespace

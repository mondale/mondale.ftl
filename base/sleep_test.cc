#include "base/sleep.h"
#include "base/time.h"
#include "testing/testing.h"

using namespace base;

namespace {

TEST(SleepForZeroTest) {
  CycleTimer ct;
  SleepFor(Duration::Zero());
  EXPECT_GT(ct.Elapsed(), Duration::Zero());
}

TEST(SleepForSmallTest) {
  CycleTimer ct;
  SleepFor(Microseconds(1));
  EXPECT_GT(ct.Elapsed(), Microseconds(1));
}

TEST(SleepForMediumTest) {
  CycleTimer ct;
  SleepFor(Milliseconds(1));
  EXPECT_GT(ct.Elapsed(), Milliseconds(1));
}

TEST(SleepUntilWallTime) {
  const WallTime start = WallTime::Now();
  const WallTime target = start + Milliseconds(1);
  SleepUntil(target);
  EXPECT_GT(WallTime::Now(), target);
}

TEST(SleepUntilMonotonicTime) {
  const MonotonicTime start = MonotonicTime::Now();
  const MonotonicTime target = start + Milliseconds(1);
  SleepUntil(target);
  EXPECT_GT(MonotonicTime::Now(), target);
}

TEST(SleepUntilCycleTime) {
  const CycleTime start = CycleTime::Now();
  const CycleTime target = start + Milliseconds(1);
  SleepUntil(target);
  EXPECT_GT(CycleTime::Now(), target);
}

}  // namespace

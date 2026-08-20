#include "base/basic_test.h"
#include "base/time.h"

using namespace base;

namespace {

void BusySpin() {
  for (volatile int i = 0; i < 10'000; (i = i + 1)) {
    continue;
  }
}

TEST(DurationFactoriesAndAccessors) {
  const Duration d_zero;
  EXPECT_EQ(d_zero.ToNanoseconds(), int64_t{0});

  const auto d_nanos = Duration::FromNanoseconds(500);
  EXPECT_EQ(d_nanos.ToNanoseconds(), int64_t{500});

  const auto d_micros = Duration::FromMicroseconds(2);
  EXPECT_EQ(d_micros.ToNanoseconds(), int64_t{2'000});

  const auto d_millis = Duration::FromMilliseconds(3);
  EXPECT_EQ(d_millis.ToNanoseconds(), int64_t{3'000'000});

  const auto d_secs = Duration::FromSeconds(4);
  EXPECT_EQ(d_secs.ToNanoseconds(), int64_t{4'000'000'000});
}

TEST(DurationArithmeticAndComparisons) {
  const auto d1 = Duration::FromMilliseconds(100);
  const auto d2 = Duration::FromMilliseconds(50);

  EXPECT_EQ((d1 + d2).ToNanoseconds(), int64_t{150'000'000});
  EXPECT_EQ((d1 - d2).ToNanoseconds(), int64_t{50'000'000});

  Duration d3 = d1;
  d3 += d2;
  EXPECT_EQ(d3.ToNanoseconds(), int64_t{150'000'000});

  d3 -= d1;
  EXPECT_EQ(d3.ToNanoseconds(), int64_t{50'000'000});

  EXPECT_LT(d2, d1);
  EXPECT_GT(d1, d2);
  EXPECT_EQ(d1, Duration::FromMilliseconds(100));
  EXPECT_NE(d1, d2);
}

TEST(WallTimeAffineArithmetic) {
  const WallTime t_epoch;
  EXPECT_EQ(t_epoch.UnixNanoseconds(), int64_t{0});

  const WallTime t1 = WallTime::Now();
  const int64_t base = t1.UnixNanoseconds();
  EXPECT_GT(base, int64_t{0});
  const auto d = Duration::FromSeconds(2);

  const WallTime t2 = t1 + d;
  EXPECT_EQ(t2.UnixNanoseconds(), int64_t{base + 2'000'000'000});

  const WallTime t3 = d + t1;
  EXPECT_EQ(t3.UnixNanoseconds(), int64_t{base + 2'000'000'000});

  const WallTime t4 = t2 - d;
  EXPECT_EQ(t4.UnixNanoseconds(), base);

  const Duration delta = t2 - t1;
  EXPECT_EQ(delta.ToNanoseconds(), d.ToNanoseconds());
}

TEST(WallTimeMutableArithmetic) {
  WallTime t_mut = WallTime::Now();
  const int64_t base = t_mut.UnixNanoseconds();
  ASSERT_GT(base, int64_t(100));
  t_mut += Duration::FromNanoseconds(200);
  EXPECT_EQ(t_mut.UnixNanoseconds(), base + int64_t{700});

  t_mut -= Duration::FromNanoseconds(300);
  EXPECT_EQ(t_mut.UnixNanoseconds(), base - 100);
}

TEST(WallTimeComparisons) {
  const WallTime t1 = WallTime::Now();
  const WallTime t2 = t1 + Duration::FromNanoseconds(2000);
  const WallTime t1_dup(t1);

  EXPECT_LT(t1, t2);
  EXPECT_GT(t2, t1);
  EXPECT_EQ(t1, t1_dup);
  EXPECT_NE(t1, t2);
}

TEST(MonotonicTimeBasic) {
  constexpr MonotonicTime m_default;
  EXPECT_EQ(m_default.nanos(), int64_t{0});

  const MonotonicTime m1 = MonotonicTime::Now();
  const MonotonicTime m1_dup(m1);
  BusySpin();
  const MonotonicTime m2 = MonotonicTime::Now();

  EXPECT_GT(m1.nanos(), int64_t{0});
  EXPECT_LE(m1, m2);
  EXPECT_GE(m2, m1);
  EXPECT_EQ(m1, m1_dup);
  EXPECT_NE(m1, m2);
}

TEST(CycleTimeHardwareQueries) {
  constexpr CycleTime c_default;
  EXPECT_EQ(c_default.value(), uint64_t{0});

  const CycleTime start = CycleTime::Now();
  BusySpin();
  const CycleTime end = CycleTime::Now();

  EXPECT_GE(end, start);
  EXPECT_LE(start, end);
  EXPECT_NE(start, end);
}

TEST(CpuFrequencyFetch) {
  const int64_t freq = CycleTime::CpuFrequencyHz();
  EXPECT_GT(freq, int64_t{0});
  EXPECT_EQ(freq, CycleTime::CpuFrequencyHz());
}

}  // namespace

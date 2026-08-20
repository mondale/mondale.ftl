#ifndef BASE_TIME_H_
#define BASE_TIME_H_

#include <time.h>

#include <compare>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <string>

namespace base {

// Represents a high-resolution duration in nanoseconds.
class Duration {
 public:
  constexpr Duration() noexcept = default;
  constexpr explicit Duration(int64_t nanos) noexcept : nanos_(nanos) {}

  [[nodiscard]] static constexpr Duration FromNanoseconds(int64_t ns) noexcept {
    return Duration(ns);
  }
  [[nodiscard]] static constexpr Duration FromMicroseconds(
      int64_t us) noexcept {
    return Duration(us * 1'000);
  }
  [[nodiscard]] static constexpr Duration FromMilliseconds(
      int64_t ms) noexcept {
    return Duration(ms * 1'000'000);
  }
  [[nodiscard]] static constexpr Duration FromSeconds(int64_t s) noexcept {
    return Duration(s * 1'000'000'000);
  }

  [[nodiscard]] constexpr int64_t ToNanoseconds() const noexcept {
    return nanos_;
  }

  constexpr Duration& operator+=(Duration rhs) noexcept {
    nanos_ += rhs.nanos_;
    return *this;
  }
  constexpr Duration& operator-=(Duration rhs) noexcept {
    nanos_ -= rhs.nanos_;
    return *this;
  }

  [[nodiscard]] friend constexpr Duration operator+(Duration lhs,
                                                    Duration rhs) noexcept {
    return Duration(lhs.nanos_ + rhs.nanos_);
  }
  [[nodiscard]] friend constexpr Duration operator-(Duration lhs,
                                                    Duration rhs) noexcept {
    return Duration(lhs.nanos_ - rhs.nanos_);
  }

  [[nodiscard]] friend constexpr auto operator<=>(Duration,
                                                  Duration) noexcept = default;

  std::string ToString() const { return std::to_string(nanos_); }

  struct timespec ToTimespec() const {
    struct timespec ts;
    if (nanos_ < 1'000'000'000) {
      ts.tv_sec = 0;
      ts.tv_nsec = nanos_;
    } else {
      ts.tv_sec = nanos_ / 1'000'000'000;
      ts.tv_nsec = nanos_ % 1'000'000'000;
    }
    return ts;
  }

 private:
  int64_t nanos_ = 0;
};

inline constexpr Duration Nanoseconds(int64_t ns) {
  return Duration::FromNanoseconds(ns);
}
inline constexpr Duration Microseconds(int64_t us) {
  return Duration::FromMicroseconds(us);
}
inline constexpr Duration Milliseconds(int64_t ms) {
  return Duration::FromMilliseconds(ms);
}
inline constexpr Duration Seconds(int64_t s) {
  return Duration::FromSeconds(s);
}

// Real-world clock time. Supports offset arithmetic with Duration.
class WallTime {
 public:
  constexpr WallTime() noexcept = default;

  [[nodiscard]] static WallTime Now() noexcept {
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return WallTime(static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 +
                    ts.tv_nsec);
  }

  [[nodiscard]] constexpr int64_t UnixNanoseconds() const noexcept {
    return nanos_since_epoch_;
  }

  constexpr WallTime& operator+=(Duration d) noexcept {
    nanos_since_epoch_ += d.ToNanoseconds();
    return *this;
  }
  constexpr WallTime& operator-=(Duration d) noexcept {
    nanos_since_epoch_ -= d.ToNanoseconds();
    return *this;
  }

  [[nodiscard]] friend constexpr WallTime operator+(WallTime wt,
                                                    Duration d) noexcept {
    return WallTime(wt.nanos_since_epoch_ + d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr WallTime operator+(Duration d,
                                                    WallTime wt) noexcept {
    return WallTime(wt.nanos_since_epoch_ + d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr WallTime operator-(WallTime wt,
                                                    Duration d) noexcept {
    return WallTime(wt.nanos_since_epoch_ - d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr Duration operator-(WallTime lhs,
                                                    WallTime rhs) noexcept {
    return Duration(lhs.nanos_since_epoch_ - rhs.nanos_since_epoch_);
  }

  [[nodiscard]] friend constexpr auto operator<=>(WallTime,
                                                  WallTime) noexcept = default;

  std::string ToString() const { return std::to_string(nanos_since_epoch_); }

 private:
  constexpr explicit WallTime(int64_t nanos) noexcept
      : nanos_since_epoch_(nanos) {}
  int64_t nanos_since_epoch_ = 0;
};

// Opaque steady system clock. Math and cross-timebase conversions disabled.
class MonotonicTime {
 public:
  constexpr MonotonicTime() noexcept = default;

  [[nodiscard]] static MonotonicTime Now() noexcept {
    timespec ts;
    ::clock_gettime(CLOCK_MONOTONIC, &ts);
    return MonotonicTime(static_cast<int64_t>(ts.tv_sec) * 1'000'000'000 +
                         ts.tv_nsec);
  }

  [[nodiscard]] constexpr int64_t nanos() const noexcept { return nanos_; }
  [[nodiscard]] friend constexpr auto operator<=>(
      MonotonicTime, MonotonicTime) noexcept = default;

  constexpr MonotonicTime& operator+=(Duration d) noexcept {
    nanos_ += d.ToNanoseconds();
    return *this;
  }
  constexpr MonotonicTime& operator-=(Duration d) noexcept {
    nanos_ -= d.ToNanoseconds();
    return *this;
  }

  [[nodiscard]] friend constexpr MonotonicTime operator+(MonotonicTime wt,
                                                         Duration d) noexcept {
    return MonotonicTime(wt.nanos_ + d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr MonotonicTime operator+(
      Duration d, MonotonicTime wt) noexcept {
    return MonotonicTime(wt.nanos_ + d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr MonotonicTime operator-(MonotonicTime wt,
                                                         Duration d) noexcept {
    return MonotonicTime(wt.nanos_ - d.ToNanoseconds());
  }
  [[nodiscard]] friend constexpr Duration operator-(
      MonotonicTime lhs, MonotonicTime rhs) noexcept {
    return Duration(lhs.nanos_ - rhs.nanos_);
  }

  std::string ToString() const { return std::to_string(nanos_); }

 private:
  constexpr explicit MonotonicTime(int64_t nanos) noexcept : nanos_(nanos) {}
  int64_t nanos_ = 0;
};

// Opaque CPU instruction cycle counter. No arithmetic allowed.
class CycleTime {
 public:
  constexpr CycleTime() noexcept = default;

  [[nodiscard]] static CycleTime Now() noexcept {
#if defined(__aarch64__)
    uint64_t val;
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(val));
    return CycleTime(val);
#elif defined(__x86_64__)
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return CycleTime((static_cast<uint64_t>(hi) << 32) | lo);
#else
#error "Unsupported architecture"
#endif
  }

  static int64_t CpuFrequencyHz();

  [[nodiscard]] constexpr uint64_t value() const noexcept { return value_; }
  [[nodiscard]] friend constexpr auto operator<=>(CycleTime,
                                                  CycleTime) noexcept = default;

  std::string ToString() const { return std::to_string(value_); }

  static uint64_t CyclesFromDuration(Duration d) noexcept {
    return static_cast<uint64_t>((static_cast<double>(d.ToNanoseconds()) *
                                  static_cast<double>(CpuFrequencyHz())) /
                                 1e9);
  }

  static Duration DurationFromCycles(uint64_t cycles) noexcept {
    const double cycles_per_ns = CpuFrequencyHz() / 1e9;
    return Nanoseconds(cycles / cycles_per_ns);
  }

  CycleTime& operator+=(Duration d) noexcept {
    value_ += CyclesFromDuration(d);
    return *this;
  }

  [[nodiscard]] friend CycleTime operator+(CycleTime t, Duration d) noexcept {
    return CycleTime(t.value_ + CyclesFromDuration(d));
  }

  [[nodiscard]] friend CycleTime operator+(Duration d, CycleTime t) noexcept {
    return CycleTime(t.value_ + CyclesFromDuration(d));
  }

  constexpr explicit CycleTime(uint64_t value) noexcept : value_(value) {}

 private:
  uint64_t value_ = 0;
};

inline std::ostream& operator<<(std::ostream& out, Duration d) {
  out << d.ToString();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, WallTime w) {
  out << w.ToString();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, MonotonicTime m) {
  out << m.ToString();
  return out;
}

inline std::ostream& operator<<(std::ostream& out, CycleTime c) {
  out << c.ToString();
  return out;
}

}  // namespace base

#endif  // #ifndef BASE_TIME_H_

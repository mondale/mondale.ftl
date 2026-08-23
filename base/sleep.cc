#include "base/sleep.h"

namespace base {
namespace {

void SpinUntil(CycleTime c) {
  while (CycleTime::Now() < c) continue;
}

}  // namespace

void SleepFor(Duration d) {
  // Spinrange.
  if (d < Microseconds(10)) {
    const auto target_cycles = CycleTime::Now() + d;
    SpinUntil(target_cycles);
    return;
  }

  // We're in sleep range.
  struct timespec ts = d.ToTimespec();
  while (-1 == ::nanosleep(&ts, &ts));
}

void SleepUntil(WallTime w) {
  const auto now = WallTime::Now();
  if (w < now) {
    return;
  }
  SleepFor(w - now);
}

void SleepUntil(MonotonicTime m) {
  const auto now = MonotonicTime::Now();
  if (m < now) {
    return;
  }
  SleepFor(m - now);
}

void SleepUntil(CycleTime c) {
  const auto now = CycleTime::Now();
  if (c < now) {
    return;
  }
  const uint64_t sleep_cycles = c.value() - now.value();
  const Duration sleep_duration = CycleTime::DurationFromCycles(sleep_cycles);
  SleepFor(sleep_duration);
}

}  // namespace base

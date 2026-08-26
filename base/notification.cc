#include <errno.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>

#include "base/notification.h"

namespace base {
namespace {

// Invokes Linux sys_futex directly.
long Futex(volatile uint32_t* uaddr, int op, uint32_t val,
           const struct timespec* timeout, uint32_t* uaddr2, uint32_t val3) {
  return ::syscall(SYS_futex, uaddr, op, val, timeout, uaddr2, val3);
}

// Blocks until *state != expected_val or the absolute monotonic timeout is
// reached. Returns true if woken/notified, false on timeout (ETIMEDOUT).
bool FutexWaitAbsolute(std::atomic<uint32_t>* state, uint32_t expected_val,
                       base::Duration d) {
  struct timespec deadline;
  // Match clock with FUTEX_CLOCK_REALTIME
  ::clock_gettime(CLOCK_REALTIME, &deadline);

  constexpr int64_t kNanosPerSec = 1'000'000'000;
  int64_t total_nanos = deadline.tv_nsec + d.ToNanoseconds();
  deadline.tv_sec += total_nanos / kNanosPerSec;
  deadline.tv_nsec = total_nanos % kNanosPerSec;

  // FUTEX_CLOCK_REALTIME expects absolute CLOCK_REALTIME in timespec
  const int op = FUTEX_WAIT_BITSET_PRIVATE | FUTEX_CLOCK_REALTIME;

  while (state->load(std::memory_order_acquire) == expected_val) {
    long ret = Futex(reinterpret_cast<volatile uint32_t*>(state), op,
                     expected_val, &deadline, nullptr, FUTEX_BITSET_MATCH_ANY);

    if (ret == 0) {
      break;
    }
    if (errno == ETIMEDOUT) {
      return false;
    }
  }

  return true;
}

}  // namespace

void Notification::Notify() {
  // Exchange state to kNotified.
  uint32_t prev = state_.exchange(kNotified, std::memory_order_release);

  // If already notified, this is a redundant call.
  if (prev == kNotified) {
    return;
  }

  // OPTIMIZATION: Only issue a kernel wake (futex) if a thread actually entered
  // the kWaiting state. If prev == kUnnotified, no waiters ever hit kernel
  // sleep.
  if (prev == kWaiting) {
    Futex(reinterpret_cast<volatile uint32_t*>(&state_), FUTEX_WAKE_PRIVATE,
          INT32_MAX, nullptr, nullptr, 0);
  }
}

void Notification::WaitForNotification() {
  uint32_t s = state_.load(std::memory_order_acquire);
  if (s == kNotified) {
    return;
  }

  if (s == kUnnotified) {
    uint32_t expected = kUnnotified;
    if (!state_.compare_exchange_strong(expected, kWaiting,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire)) {
      if (expected == kNotified) {
        return;
      }
    }
  }

  while ((s = state_.load(std::memory_order_acquire)) == kWaiting) {
    Futex(reinterpret_cast<volatile uint32_t*>(&state_), FUTEX_WAIT_PRIVATE,
          kWaiting, nullptr, nullptr, 0);
  }
}

bool Notification::WaitForNotificationWithTimeout(base::Duration d) {
  uint32_t s = state_.load(std::memory_order_acquire);
  if (s == kNotified) {
    return true;
  }

  if (s == kUnnotified) {
    uint32_t expected = kUnnotified;
    if (!state_.compare_exchange_strong(expected, kWaiting,
                                        std::memory_order_acq_rel,
                                        std::memory_order_acquire)) {
      if (expected == kNotified) {
        return true;
      }
    }
  }

  if (!FutexWaitAbsolute(&state_, kWaiting, d)) {
    // Re-check state on timeout in case Notify() raced with timeout expiration
    return state_.load(std::memory_order_acquire) == kNotified;
  }

  return state_.load(std::memory_order_acquire) == kNotified;
}

}  // namespace base

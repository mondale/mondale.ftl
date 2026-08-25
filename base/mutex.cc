#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "base/mutex.h"
#include "base/time.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>  // for _mm_pause()
#endif

namespace base {
namespace {

void CpuRelax() {
#if defined(__x86_64__) || defined(_M_X64)
  _mm_pause();
#elif defined(__aarch64__)
  asm volatile("yield" ::: "memory");
#endif
}

int FutexWaitTimeout(std::atomic<uint32_t>* addr, uint32_t val, Duration d) {
  struct timespec ts = d.ToTimespec();
  return syscall(SYS_futex, reinterpret_cast<uint32_t*>(addr),
                 FUTEX_WAIT_PRIVATE, val, &ts, nullptr, 0);
}

}  // namespace

void Mutex::LockSlow() {
  constexpr int kSpinLimit = 36;
  int spins = 0;

  uint32_t c = state_.load(std::memory_order_relaxed);

  while (true) {
    // 1. Spin phase if uncontended
    if (c == kLockedUncontended && spins < kSpinLimit) {
      CpuRelax();
      spins++;
      c = state_.load(std::memory_order_relaxed);
      continue;
    }

    // 2. Try to acquire lock.
    // If it's unlocked (0), we set state to 2 (kLockedContended) because
    // we are in the slow path where other waiters may be waiting or incoming.
    // If c was already 2, setting it to 2 preserves waiter status for Unlock().
    if (c == kUnlocked || c == kLockedUncontended) {
      if (state_.exchange(kLockedContended, std::memory_order_acquire) ==
          kUnlocked) {
        return;
      }
    }

    // 3. Wait phase.
    // At this point state_ is set to 2. Wait until state_ != 2.
    state_.wait(kLockedContended, std::memory_order_relaxed);

    // 4. Reload state after wake-up / spurious return
    c = state_.load(std::memory_order_relaxed);
  }
}

void Mutex::WaitSlow() {
  // Mark as contended so the thread holding the lock knows to wake us up.
  state_.store(kLockedContended, std::memory_order_relaxed);
  Unlock();
  state_.wait(kLockedContended, std::memory_order_relaxed);
  Lock();
}

void Mutex::WaitWithTimeoutSlow(Duration d) {
  Unlock();
  // Call Linux futex directly since std::atomic lacks wait_for.
  FutexWaitTimeout(&state_, kUnlocked, d);
  Lock();
}

}  // namespace base

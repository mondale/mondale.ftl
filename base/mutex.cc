#include "base/mutex.h"

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>  // for _mm_pause()
#endif

namespace base {
namespace {

void CpuRelax() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
  _mm_pause();
#elif defined(__aarch64__)
  asm volatile("yield" ::: "memory");
#endif
}
}  // namespace

void Mutex::LockSlow() noexcept {
  constexpr int kSpinLimit = 36;  // arbitrary
  int spins = 0;

  while (true) {
    uint32_t current = state_.load(std::memory_order_relaxed);

    // 1. Adaptive Spin Phase
    // If the lock is held uncontended (1), spin briefly in user space
    // in case the holding thread is about to release it.
    if (current == kLockedUncontended && spins < kSpinLimit) {
      CpuRelax();
      spins++;
      continue;
    }

    // 2. Mark state as CONTENDED (2) if it isn't already 0
    if (current != kLockedContended) {
      current = state_.exchange(kLockedContended, std::memory_order_relaxed);
      if (current == kUnlocked) {
        // Acquired lock during transition!
        std::atomic_thread_fence(std::memory_order_acquire);
        return;
      }
    }

    // 3. Futex Wait Phase
    // Suspends thread in kernel if state_ == kLockedContended
    state_.wait(kLockedContended, std::memory_order_relaxed);

    // 4. On Wakeup / Spurious Return
    // Try to acquire lock directly assuming contender status (0 -> 2)
    current = kUnlocked;
    if (state_.compare_exchange_strong(current, kLockedContended,
                                       std::memory_order_acquire,
                                       std::memory_order_relaxed)) {
      return;
    }
  }
}

}  // namespace base

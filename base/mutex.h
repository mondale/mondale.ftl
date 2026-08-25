#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include <atomic>
#include <chrono>
#include <cstdint>

#include "base/thread_annotations.h"
#include "base/time.h"

namespace base {

// A mutex built from a single DWORD, which aligns with what Linux's futex
// provides. For general purpose use in most applications.
class CAPABILITY("mutex") Mutex {
 public:
  Mutex() = default;
  ~Mutex() = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  void Lock() ACQUIRE() {
    if (TryLock()) return;
    LockSlow();
  }

  bool TryLock() TRY_ACQUIRE(true) {
    // Fast path: attempt to claim uncontended lock (kUnlocked ->
    // kLockedUncontended)
    uint32_t expected = kUnlocked;
    return state_.compare_exchange_strong(expected, kLockedUncontended,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  void Unlock() RELEASE() {
    // Release the lock state
    uint32_t prev = state_.exchange(kUnlocked, std::memory_order_release);

    // If there were waiters (state was kLockedContended), wake up all threads
    // so waiters evaluating predicates can re-check their condition.
    if (prev == kLockedContended) {
      state_.notify_all();
    }
  }

  // Blocks until `pred` evaluates to true. Mutex must be held by the caller.
  template <typename Predicate>
  void Await(Predicate fn) LOCKS_REQUIRED(this) {
    while (!fn()) {
      WaitSlow();
    }
  }

  // Blocks until `pred` evaluates to true or `d` elapses.
  // Returns true if the condition was met, false if timed out.
  // Mutex must be held by the caller.
  template <typename Predicate>
  bool AwaitWithTimeout(Predicate fn, Duration d) LOCKS_REQUIRED(this) {
    const auto deadline = base::MonotonicTime::Now() + d;
    while (!fn()) {
      const auto now = base::MonotonicTime::Now();
      if (now >= deadline) {
        return fn();
      }
      WaitWithTimeoutSlow(deadline - now);
    }
    return true;
  }

 private:
  // Unlocked.
  static constexpr uint32_t kUnlocked = 0;

  // Locked, no waiters.
  static constexpr uint32_t kLockedUncontended = 1;

  // Locked, has waiters.
  static constexpr uint32_t kLockedContended = 2;

  void LockSlow();
  void WaitSlow() LOCKS_REQUIRED(this);
  void WaitWithTimeoutSlow(Duration d) LOCKS_REQUIRED(this);

  std::atomic<uint32_t> state_{0};
};

// RAII lock guard for base::Mutex.
class SCOPED_CAPABILITY MutexLock {
 public:
  explicit MutexLock(Mutex* mu) ACQUIRE(mu) : mu_(mu) { mu_->Lock(); }

  ~MutexLock() RELEASE() { mu_->Unlock(); }

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;

 private:
  Mutex* const mu_;
};

}  // namespace base

#endif  // BASE_MUTEX_H_

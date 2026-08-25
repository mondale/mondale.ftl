#ifndef BASE_MUTEX_H_
#define BASE_MUTEX_H_

#include <atomic>
#include <cstdint>

namespace base {

// A mutex built from a single DWORD, which aligns with what Linux's futex
// provides. For general purpose use in most applications.
class Mutex {
 public:
  Mutex() noexcept = default;
  ~Mutex() noexcept = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  void Lock() noexcept {
    if (TryLock()) return;
    LockSlow();
  }

  bool TryLock() noexcept {
    // Fast path: attempt to claim uncontended lock (kUnlocked ->
    // kLockedUncontended)
    uint32_t expected = kUnlocked;
    return state_.compare_exchange_strong(expected, kLockedUncontended,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  void Unlock() noexcept {
    // Release the lock state
    uint32_t prev = state_.exchange(kUnlocked, std::memory_order_release);

    // If there were waiters (state was kLockedContended), wake up exactly one
    // thread.
    if (prev == kLockedContended) {
      state_.notify_one();  // Translates to futex(FUTEX_WAKE, 1)
    }
  }

 private:
  // Unlocked.
  static constexpr uint32_t kUnlocked = 0;

  // Locked, no waiters.
  static constexpr uint32_t kLockedUncontended = 1;

  // Locked, has waiters.
  static constexpr uint32_t kLockedContended = 2;

  void LockSlow() noexcept;

  std::atomic<uint32_t> state_{0};
};

}  // namespace base

#endif  // #ifndef BASE_MUTEX_H_

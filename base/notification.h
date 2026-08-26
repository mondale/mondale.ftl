#ifndef BASE_NOTIFICATION_H_
#define BASE_NOTIFICATION_H_

#include <atomic>
#include <cstdint>

#include "base/time.h"

namespace base {

// A single-use synchronization primitive that allows one or more threads to
// wait until a notification is triggered.
class Notification {
 public:
  Notification() = default;
  ~Notification() = default;

  Notification(const Notification&) = delete;
  Notification& operator=(const Notification&) = delete;

  // Returns true if Notify() has been called.
  bool HasBeenNotified() const {
    return state_.load(std::memory_order_acquire) == kNotified;
  }

  // Triggers the notification and wakes all waiting threads.
  // Idempotent. Only the first call transitions state and conditionally
  // signals waiters.
  void Notify();

  // Blocks the calling thread until Notify() is called.
  void WaitForNotification();

  // Blocks the calling thread until Notify() is called or `d` elapses.
  // Returns true if notified, false on timeout.
  bool WaitForNotificationWithTimeout(base::Duration d);

 private:
  // State 0: Initial state, no waiters.
  static constexpr uint32_t kUnnotified = 0;

  // State 1: Notification has been triggered.
  static constexpr uint32_t kNotified = 1;

  // State 2: At least one thread is actively waiting on the futex.
  static constexpr uint32_t kWaiting = 2;

  std::atomic<uint32_t> state_{kUnnotified};
};

}  // namespace base

#endif  // BASE_NOTIFICATION_H_

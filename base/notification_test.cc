#include <vector>

#include "base/notification.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

namespace base {

TEST(NotificationBasic) {
  Notification n;
  EXPECT_FALSE(n.HasBeenNotified());

  n.Notify();
  EXPECT_TRUE(n.HasBeenNotified());

  n.Notify();
  EXPECT_TRUE(n.HasBeenNotified());
}

TEST(NotificationNoWaitersFastPath) {
  Notification n;

  n.Notify();
  EXPECT_TRUE(n.HasBeenNotified());

  n.WaitForNotification();
  EXPECT_TRUE(n.HasBeenNotified());
}

TEST(NotificationWaitSingleThread) {
  Notification n;

  auto t = CreateThread("notifier", [&n]() { n.Notify(); });

  n.WaitForNotification();
  EXPECT_TRUE(n.HasBeenNotified());
  t->Join();
}

TEST(NotificationWaitMultipleWaiters) {
  Notification n;
  constexpr int kNumWaiters = 8;
  std::atomic<int> notified_count{0};

  std::vector<std::unique_ptr<Thread>> waiters;
  waiters.reserve(kNumWaiters);

  for (int i = 0; i < kNumWaiters; ++i) {
    waiters.push_back(CreateThread("waiter", [&n, &notified_count]() {
      n.WaitForNotification();
      notified_count.fetch_add(1, std::memory_order_relaxed);
    }));
  }

  EXPECT_EQ(notified_count.load(), 0);
  n.Notify();

  for (auto& t : waiters) {
    t->Join();
  }

  EXPECT_EQ(notified_count.load(), kNumWaiters);
}

TEST(NotificationTimeoutExpired) {
  Notification n;

  bool result = n.WaitForNotificationWithTimeout(base::Milliseconds(5));
  EXPECT_FALSE(result);
  EXPECT_FALSE(n.HasBeenNotified());
}

TEST(NotificationTimeoutSatisfied) {
  Notification n;

  auto t = CreateThread("notifier", [&n]() { n.Notify(); });

  bool result = n.WaitForNotificationWithTimeout(base::Seconds(200));
  EXPECT_TRUE(result);
  EXPECT_TRUE(n.HasBeenNotified());
  t->Join();
}

}  // namespace base

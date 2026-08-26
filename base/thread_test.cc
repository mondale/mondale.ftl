#include <atomic>

#include "core/vocabulary.h"
#include "testing/testing.h"

namespace {

TEST(MakeMeAThread) {
  std::atomic<pid_t> tid{0};
  auto thread = base::CreateThread("my_thread", [&]() {
    tid.store(GetCachedTid(), std::memory_order_release);
  });
  while (0 == tid.load(std::memory_order_acquire) || !thread->ReadyToJoin()) {
    SleepFor(Milliseconds(1));
  }
  ASSERT_TRUE(thread->ReadyToJoin());
  thread->Join();
  thread->Join();  // obnoxiously do this more than once.
}

TEST(DetachMeAThread) {
  std::atomic<bool> ran{false};
  base::CreateDetachedThread(
      "my_thread", [&]() { ran.store(true, std::memory_order_release); });
  while (!ran.load(std::memory_order_acquire)) {
    SleepFor(Milliseconds(1));
  }
}

}  // namespace

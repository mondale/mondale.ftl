#include <atomic>

#include "core/vocabulary.h"
#include "testing/testing.h"

namespace {

TEST(MakeMeAThread) {
  std::atomic<pid_t> tid{0};
  auto thread = core::CreateThread("my_thread", [&]() {
                  tid.store(gettid(), std::memory_order_release);
                }).ValueOrDie();
  while (0 == tid.load(std::memory_order_acquire) || !thread->ReadyToJoin()) {
    SleepFor(Milliseconds(1));
  }
  ASSERT_TRUE(thread->ReadyToJoin());
  thread->Join();
  thread->Join();  // obnoxiously do this more than once.
}

TEST(DetachMeAThread) {
  std::atomic<bool> ran{false};
  EXPECT_EQ(Code::kOk, core::CreateDetachedThread("my_thread", [&]() {
                         ran.store(true, std::memory_order_release);
                       }).code());
  while (!ran.load(std::memory_order_acquire)) {
    SleepFor(Milliseconds(1));
  }
}

}  // namespace

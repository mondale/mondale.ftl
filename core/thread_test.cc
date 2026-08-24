#include <atomic>

#include "core/vocabulary.h"
#include "testing/testing.h"

namespace {

TEST(MakeMeAThread) {
  std::atomic<bool> ran{false};
  auto thread = core::CreateThread("my_thread", [&]() {
                  ran.store(true, std::memory_order_release);
                }).ValueOrDie();
  while (!ran.load(std::memory_order_acquire) || !thread->ReadyToJoin()) {
    SleepFor(Milliseconds(1));
  }
  ASSERT_TRUE(thread->ReadyToJoin());
  thread->Join();
  thread->Join();  // obnoxiously do this more than once.
}

}  // namespace

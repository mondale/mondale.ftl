#include <atomic>
#include <list>

#include "base/mutex.h"
#include "core/vocabulary.h"
#include "testing/testing.h"

namespace base {
namespace {

TEST(BasicLockUnlock) {
  Mutex m;
  m.Lock();
  m.Unlock();
}

TEST(TryLockSuccessAndFailure) {
  Mutex m;

  EXPECT_TRUE(m.TryLock());
  EXPECT_FALSE(m.TryLock());

  m.Unlock();

  EXPECT_TRUE(m.TryLock());
  m.Unlock();
}

TEST(SequentialLocking) {
  Mutex m;

  for (int i = 0; i < 100; ++i) {
    m.Lock();
    m.Unlock();
  }
}

TEST(MutualExclusion) {
  Mutex m;
  int counter = 0;
  constexpr int kIncrementsPerThread = 10000;

  auto worker = [&]() {
    for (int i = 0; i < kIncrementsPerThread; ++i) {
      m.Lock();
      ++counter;
      m.Unlock();
    }
  };

  auto t1 = CreateThread("test", worker);
  auto t2 = CreateThread("test", worker);

  t1->Join();
  t2->Join();

  EXPECT_EQ(counter, kIncrementsPerThread * 2);
}

TEST(HighContention) {
  Mutex m;
  int counter = 0;
  constexpr int kNumThreads = 8;
  constexpr int kIncrementsPerThread = 5000;

  auto worker = [&]() {
    for (int i = 0; i < kIncrementsPerThread; ++i) {
      m.Lock();
      ++counter;
      m.Unlock();
    }
  };

  std::list<std::unique_ptr<Thread>> threads;

  for (int i = 0; i < kNumThreads; ++i) {
    auto t = CreateThread("test", worker);
    threads.emplace_back(std::move(t));
  }

  for (auto& t : threads) {
    t->Join();
  }

  EXPECT_EQ(counter, kNumThreads * kIncrementsPerThread);
}

TEST(TryLockContention) {
  Mutex m;
  std::atomic<bool> thread_started{false};
  std::atomic<bool> hold_lock{true};

  m.Lock();

  auto worker = [&]() {
    thread_started.store(true, std::memory_order_release);
    // TryLock should fail while main thread holds m.
    EXPECT_FALSE(m.TryLock());

    while (hold_lock.load(std::memory_order_acquire)) {
      // Wait for main thread to release lock.
    }

    // Eventually acquire lock once released.
    m.Lock();
    m.Unlock();
  };

  auto t = CreateThread("test", worker);

  while (!thread_started.load(std::memory_order_acquire)) {
    // Wait for thread to attempt TryLock.
  }

  hold_lock.store(false, std::memory_order_release);
  m.Unlock();
}

}  // namespace
}  // namespace base

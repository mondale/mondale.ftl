#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include "base/log_queue.h"
#include "base/thread.h"
#include "testing/testing.h"

using base::internal::LogEntry;
using base::internal::LogSeverity;

namespace base {

TEST(LogQueueBasicPushPop) {
  int poke_count = 0;
  LogQueue queue([&poke_count]() { ++poke_count; });

  LogEntry entry_in{.severity = LogSeverity::kInfo};
  queue.Push(entry_in);

  LogEntry entry_out;
  EXPECT_TRUE(queue.Pop(&entry_out));
  EXPECT_TRUE(entry_out.severity == LogSeverity::kInfo);
  EXPECT_EQ(poke_count, 0);

  EXPECT_FALSE(queue.Pop(&entry_out));
}

TEST(LogQueueThresholdPokeTrigger) {
  int poke_count = 0;
  LogQueue queue([&poke_count]() { ++poke_count; });

  // Fill below threshold (kThreshold = 8192)
  for (int i = 0; i < 8192; ++i) {
    queue.Push(LogEntry{.severity = LogSeverity::kInfo});
  }
  EXPECT_EQ(poke_count, 0);

  // Cross threshold (8193rd item)
  queue.Push(LogEntry{.severity = LogSeverity::kWarning});
  EXPECT_EQ(poke_count, 1);

  // Next pushes past threshold continue poking
  queue.Push(LogEntry{.severity = LogSeverity::kWarning});
  EXPECT_EQ(poke_count, 2);
}

TEST(LogQueueDropTrackingAndPoke) {
  int poke_count = 0;
  LogQueue queue([&poke_count]() { ++poke_count; });

  // Completely fill the queue (Capacity = 16384)
  for (size_t i = 0; i < LogQueue::kCapacity; ++i) {
    queue.Push(LogEntry{.severity = LogSeverity::kInfo});
  }

  int pokes_before_overflow = poke_count;
  EXPECT_GT(pokes_before_overflow, 0);  // Poked for every entry > 8192

  // Push overflow entries per severity
  queue.Push(LogEntry{.severity = LogSeverity::kWarning});
  queue.Push(LogEntry{.severity = LogSeverity::kWarning});
  queue.Push(LogEntry{.severity = LogSeverity::kError});

  EXPECT_EQ(queue.dropped_count(LogSeverity::kInfo), 0);
  EXPECT_EQ(queue.dropped_count(LogSeverity::kWarning), 2);
  EXPECT_EQ(queue.dropped_count(LogSeverity::kError), 1);
  EXPECT_EQ(queue.dropped_count(LogSeverity::kFatal), 0);

  // Dropping items must also trigger pokes
  EXPECT_EQ(poke_count, pokes_before_overflow + 3);
}

TEST(LogQueueMultiProducerConcurrentPushes) {
  constexpr size_t kNumProducers = 4;
  constexpr int kItemsPerProducer = 2000;  // Total: 8000 (below capacity)
  constexpr size_t kNumSeverities = static_cast<size_t>(LogSeverity::kFatal);

  std::atomic<int> poke_count{0};
  LogQueue queue(
      [&poke_count]() { poke_count.fetch_add(1, std::memory_order_relaxed); });

  std::atomic<bool> start_signal{false};
  std::vector<std::unique_ptr<Thread>> producers;
  producers.reserve(kNumProducers);

  for (size_t p = 0; p < kNumProducers; ++p) {
    producers.push_back(CreateThread([&, p]() {
      while (!start_signal.load(std::memory_order_relaxed)) {
        // Spin lock until test unblocks producers
      }

      // Cycle through valid severity levels safely
      LogSeverity severity = static_cast<LogSeverity>(p % kNumSeverities);
      for (int i = 0; i < kItemsPerProducer; ++i) {
        queue.Push(LogEntry{.severity = severity});
      }
    }));
  }

  start_signal.store(true, std::memory_order_release);

  for (auto& producer : producers) {
    producer->Join();
  }

  int consumed_count = 0;
  std::array<int, kNumSeverities> severity_counts{};
  LogEntry out;

  while (queue.Pop(&out)) {
    ++consumed_count;
    size_t idx = static_cast<size_t>(out.severity);
    if (idx < kNumSeverities) {
      severity_counts[idx]++;
    }
  }

  constexpr int kExpectedTotal = kNumProducers * kItemsPerProducer;
  EXPECT_EQ(consumed_count, kExpectedTotal);

  for (size_t i = 0; i < kNumSeverities; ++i) {
    EXPECT_EQ(queue.dropped_count(static_cast<LogSeverity>(i)), 0);
  }
}

}  // namespace base

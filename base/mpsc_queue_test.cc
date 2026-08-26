#include <atomic>
#include <concepts>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

#include "base/mpsc_queue.h"
#include "base/sleep.h"
#include "base/thread.h"
#include "base/time.h"
#include "testing/testing.h"

namespace {

void Yield() { base::SleepFor(base::Microseconds(20)); }

}  // namespace

namespace base {

TEST(MpscQueueSingleThreadedBasic) {
  MpscQueue<int, 4> queue;
  int out = 0;

  EXPECT_FALSE(queue.Dequeue(&out));

  EXPECT_TRUE(queue.TryEnqueue(10));
  EXPECT_TRUE(queue.TryEnqueue(20));

  EXPECT_TRUE(queue.Dequeue(&out));
  EXPECT_EQ(out, 10);
  EXPECT_TRUE(queue.Dequeue(&out));
  EXPECT_EQ(out, 20);

  EXPECT_FALSE(queue.Dequeue(&out));
}

TEST(MpscQueueCapacityBoundaries) {
  MpscQueue<int, 3> queue;
  int out = 0;

  EXPECT_TRUE(queue.TryEnqueue(1));
  EXPECT_TRUE(queue.TryEnqueue(2));
  EXPECT_TRUE(queue.TryEnqueue(3));
  EXPECT_FALSE(queue.TryEnqueue(4));  // Full

  EXPECT_TRUE(queue.Dequeue(&out));
  EXPECT_EQ(out, 1);

  // Slot freed, enqueue should succeed again
  EXPECT_TRUE(queue.TryEnqueue(4));
  EXPECT_FALSE(queue.TryEnqueue(5));
}

TEST(MpscQueueOccupancyOutput) {
  MpscQueue<int, 4> queue;
  int64_t occupancy = 0;
  int out = 0;

  EXPECT_TRUE(queue.TryEnqueue(100, &occupancy));
  EXPECT_EQ(occupancy, 1);

  EXPECT_TRUE(queue.TryEnqueue(200, &occupancy));
  EXPECT_EQ(occupancy, 2);

  EXPECT_TRUE(queue.TryEnqueue(300, &occupancy));
  EXPECT_EQ(occupancy, 3);

  EXPECT_TRUE(queue.Dequeue(&out));
  EXPECT_EQ(out, 100);

  EXPECT_TRUE(queue.TryEnqueue(400, &occupancy));
  EXPECT_EQ(occupancy, 3);
}

TEST(MpscQueueMoveOnlyType) {
  struct MoveOnly {
    MoveOnly() = default;
    explicit MoveOnly(int v) : value(v) {}
    MoveOnly(MoveOnly&&) = default;
    MoveOnly& operator=(MoveOnly&&) = default;
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    int value;
  };

  MpscQueue<MoveOnly, 4> queue;
  EXPECT_TRUE(queue.TryEnqueue(MoveOnly(100)));

  MoveOnly out(0);
  EXPECT_TRUE(queue.Dequeue(&out));
  EXPECT_EQ(out.value, 100);
}

TEST(MpscQueueConceptConstraints) {
  static_assert(MpscQueueElement<int>);
  static_assert(MpscQueueElement<std::unique_ptr<int>>);

  struct NonMovable {
    NonMovable(const NonMovable&) = delete;
    NonMovable(NonMovable&&) = delete;
  };
  static_assert(!MpscQueueElement<NonMovable>);
}

TEST(MpscQueueMultiProducerSingleConsumer) {
  constexpr size_t kNumProducers = 4;
  constexpr int kItemsPerProducer = 10000;
  constexpr size_t kQueueCapacity = 1024;

  MpscQueue<int64_t, kQueueCapacity> queue;
  std::atomic<bool> start_signal{false};
  std::atomic<int64_t> total_enqueued{0};
  std::atomic<int64_t> total_drops{0};

  std::vector<std::unique_ptr<Thread>> producers;
  producers.reserve(kNumProducers);

  for (size_t p = 0; p < kNumProducers; ++p) {
    producers.push_back(CreateThread("producer", [&, p]() {
      while (!start_signal.load(std::memory_order_relaxed)) {
        // Spin until start
      }

      for (int i = 0; i < kItemsPerProducer; ++i) {
        int64_t val = (static_cast<int64_t>(p) << 32) | i;
        while (!queue.TryEnqueue(val)) {
          total_drops.fetch_add(1, std::memory_order_relaxed);
          Yield();
        }
        total_enqueued.fetch_add(1, std::memory_order_relaxed);
      }
    }));
  }

  // Release producers
  start_signal.store(true, std::memory_order_release);

  int64_t consumed_count = 0;
  int64_t val = 0;
  const int64_t expected_total = kNumProducers * kItemsPerProducer;

  while (consumed_count < expected_total) {
    if (queue.Dequeue(&val)) {
      ++consumed_count;
    } else {
      Yield();
    }
  }

  for (auto& t : producers) {
    t->Join();
  }

  EXPECT_EQ(consumed_count, expected_total);
  EXPECT_EQ(total_enqueued.load(), expected_total);
}

}  // namespace base

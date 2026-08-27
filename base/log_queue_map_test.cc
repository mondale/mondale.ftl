#include <memory>
#include <vector>

#include "base/log_queue_map.h"
#include "testing/testing.h"

namespace base::internal {

class LogQueue final {};

class MockQueues final {
 public:
  MockQueues(int count) {
    for (int i = 0; i < count; ++i) {
      // Allocate 1 byte of memory per dummy queue; standard delete handles it
      // safely
      queues_.push_back(std::unique_ptr<LogQueue>(new LogQueue()));
    }
  }

  auto& queues() const { return queues_; }

 private:
  std::vector<std::unique_ptr<LogQueue>> queues_;
};

TEST(OneCpuOneQueueOneDomain) {
  NumaMap map = NumaMap::BuildFrom({0});
  MockQueues queues(1);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  EXPECT_EQ(qs[0].get(), lqm.QueueForCpu(0));
}

TEST(TwoCpusOneQueueOneDomain) {
  NumaMap map = NumaMap::BuildFrom({0, 0});
  MockQueues queues(1);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  EXPECT_EQ(qs[0].get(), lqm.QueueForCpu(0));
  EXPECT_EQ(qs[0].get(), lqm.QueueForCpu(1));
}

TEST(EightCpusEightQueuesOneDomain) {
  NumaMap map = NumaMap::BuildFrom({0, 0, 0, 0, 0, 0, 0, 0});
  MockQueues queues(8);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(qs[i].get(), lqm.QueueForCpu(i));
  }
}

TEST(EightCpusTwoQueuesOneDomain) {
  NumaMap map = NumaMap::BuildFrom({0, 0, 0, 0, 0, 0, 0, 0});
  MockQueues queues(2);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  for (int i = 0; i < 8; ++i) {
    EXPECT_EQ(qs[i % 2].get(), lqm.QueueForCpu(i));
  }
}

TEST(SixteenCpusFourQueuesTwoDomains) {
  std::vector<int> topology = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1};
  NumaMap map = NumaMap::BuildFrom(topology);
  MockQueues queues(4);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  for (int i = 0; i < 16; ++i) {
    int domain = topology[i];
    int expected_queue_idx = (domain * 2) + (i % 2);
    EXPECT_EQ(qs[expected_queue_idx].get(), lqm.QueueForCpu(i));
  }
}

TEST(SixteenCpusTwoQueuesSixteenDomains) {
  std::vector<int> topology(16);
  for (int i = 0; i < 16; ++i) topology[i] = i;

  NumaMap map = NumaMap::BuildFrom(topology);
  MockQueues queues(2);
  const auto& qs = queues.queues();

  LogQueueMap lqm(qs, map);
  for (int i = 0; i < 16; ++i) {
    int expected_queue_idx = i / 8;
    EXPECT_EQ(qs[expected_queue_idx].get(), lqm.QueueForCpu(i));
  }
}

}  // namespace base::internal

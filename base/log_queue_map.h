#ifndef BASE_LOG_QUEUE_MAP_H_
#define BASE_LOG_QUEUE_MAP_H_

#include <memory>
#include <vector>

#include "base/cpu.h"

namespace base::internal {

class LogQueue;

class LogQueueMap final {
 public:
  LogQueueMap(const std::vector<std::unique_ptr<LogQueue>>& queues,
              const NumaMap& m);

  LogQueue* QueueForThisCpu() const;
  LogQueue* QueueForCpu(int cpu) const { return map_[cpu]; }

 private:
  const std::vector<LogQueue*> map_;
};

}  // namespace base::internal

#endif  // #ifndef BASE_LOG_QUEUE_MAP_H_

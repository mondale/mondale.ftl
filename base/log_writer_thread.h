#ifndef BASE_LOG_WRITER_THREAD_H_
#define BASE_LOG_WRITER_THREAD_H_

#include <array>
#include <atomic>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/log_queue.h"
#include "base/log_queue_map.h"
#include "base/logging_internal.h"
#include "base/mutex.h"
#include "base/notification.h"

namespace base::internal {

constexpr size_t kNumSeverities = static_cast<size_t>(LogSeverity::kFatal) + 1;
constexpr size_t kMaxChunkSizeBytes = 32768;  // 32 KB per chunk
constexpr size_t kMaxSinkBufferedBytes =
    10 * 1024 * 1024;  // 10 MB limit per sink
constexpr size_t kMaxEntriesPerQueuePerCycle = 1000;

struct SinkState {
  int fd;
  uint8_t severity_mask = 0;

  std::list<std::string> pending_buffers;
  size_t total_buffered_bytes = 0;
  int64_t sink_dropped_count = 0;

  // last_reported_drops[queue_idx][severity_idx]
  std::vector<std::array<int64_t, kNumSeverities>> last_reported_drops;
};

class LogWriterThread final {
 public:
  ~LogWriterThread();

  static void Init(std::vector<std::unique_ptr<LogQueue>> queues,
                   std::vector<SinkState> sinks);

  static LogWriterThread* Instance();
  static std::function<void()> PokeFunction();

  void Stop();
  void Poke();

  LogQueue* QueueForCpu(int cpu) const { return lqm_.QueueForCpu(cpu); }

  // Returns when all previously-enqueued elements have been pulled from
  // LogQueues. Not a firm guarantee that logging is on disk.
  void Flush();

 private:
  LogWriterThread(std::vector<std::unique_ptr<LogQueue>> queues,
                  std::vector<SinkState> sinks);

  void RunLoop();

  void CheckAndReportDrops();
  void CheckDropsForQueue(size_t q_idx);
  void CheckDropsForSeverity(size_t q_idx, LogSeverity sev, int severity_idx);

  bool DrainQueues();
  void HandleFlush(const LogEntry& entry);
  void ProcessAndRouteEntry(const LogEntry& entry);
  void AppendToSink(SinkState& sink, const std::string& formatted);

  bool FlushBuffers();
  bool WriteSinkBuffer(SinkState& sink);

  void DrainRemainingQueues();
  bool AcceptsSeverity(const SinkState& sink, LogSeverity sev) const;
  void CloseFiles();

  const LogQueueMap lqm_;
  std::vector<std::unique_ptr<LogQueue>> queues_;
  std::vector<SinkState> sinks_;
  base::Notification stop_notification_;
  std::atomic<int64_t> poke_generation_{0};
  Mutex poke_mu_;  // guards writing to poke_generation_;
};

std::string GetLogPath();

}  // namespace base::internal

#endif  // BASE_LOG_WRITER_THREAD_H_

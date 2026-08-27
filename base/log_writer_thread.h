#ifndef BASE_LOG_WRITER_THREAD_H_
#define BASE_LOG_WRITER_THREAD_H_

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/log_queue.h"
#include "base/logging_internal.h"
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

  void Stop();

 private:
  LogWriterThread(std::vector<std::unique_ptr<LogQueue>> queues,
                  std::vector<SinkState> sinks);

  void RunLoop();

  void CheckAndReportDrops();
  void CheckDropsForQueue(size_t q_idx);
  void CheckDropsForSeverity(size_t q_idx, LogSeverity sev, int severity_idx);

  bool DrainQueues();
  void ProcessAndRouteEntry(const LogEntry& entry);
  void AppendToSink(SinkState& sink, const std::string& formatted);

  bool FlushBuffers();
  bool WriteSinkBuffer(SinkState& sink);

  void DrainRemainingQueues();
  bool AcceptsSeverity(const SinkState& sink, LogSeverity sev) const;

  std::vector<std::unique_ptr<LogQueue>> queues_;
  base::Notification stop_notification_;
  std::vector<SinkState> sinks_;
};

}  // namespace base::internal

#endif  // BASE_LOG_WRITER_THREAD_H_

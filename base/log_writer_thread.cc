#include <fcntl.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <system_error>

#include "base/log_writer_thread.h"
#include "base/sleep.h"
#include "base/thread.h"
#include "base/time.h"

namespace base::internal {

namespace {
LogWriterThread* g_log_writer_thread = nullptr;

void MakeNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags != -1) {
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
  }
}
}  // namespace

LogWriterThread::LogWriterThread(std::vector<std::unique_ptr<LogQueue>> queues,
                                 std::vector<SinkState> sinks)
    : queues_(std::move(queues)), sinks_(std::move(sinks)) {
  for (auto& sink : sinks_) {
    MakeNonBlocking(sink.fd);
  }
}

LogWriterThread::~LogWriterThread() { Stop(); }

void LogWriterThread::Init(std::vector<std::unique_ptr<LogQueue>> queues,
                           std::vector<SinkState> sinks) {
  if (g_log_writer_thread) {
    return;
  }

  g_log_writer_thread =
      new LogWriterThread(std::move(queues), std::move(sinks));

  CreateDetachedThread([]() { g_log_writer_thread->RunLoop(); });
}

LogWriterThread* LogWriterThread::Instance() { return g_log_writer_thread; }

void LogWriterThread::Stop() { stop_notification_.Notify(); }

bool LogWriterThread::AcceptsSeverity(const SinkState& sink,
                                      LogSeverity sev) const {
  int severity_idx = static_cast<int>(sev);
  return (sink.severity_mask & (1 << severity_idx)) != 0;
}

void LogWriterThread::CheckDropsForSeverity(size_t q_idx, LogSeverity sev,
                                            int severity_idx) {
  auto* queue = queues_[q_idx].get();
  int64_t current_drops = queue->dropped_count(sev);

  for (size_t sink_idx = 0; sink_idx < sinks_.size(); ++sink_idx) {
    if (!AcceptsSeverity(sinks_[sink_idx], sev)) {
      continue;
    }

    int64_t& last_drops =
        sinks_[sink_idx].last_reported_drops[q_idx][severity_idx];
    if (current_drops > last_drops) {
      int64_t delta = current_drops - last_drops;
      last_drops = current_drops;

      char drop_msg[128];
      int len = std::snprintf(
          drop_msg, sizeof(drop_msg),
          "*** DROPPED %lld LOG MESSAGES (Severity %d) DUE TO FULL QUEUE ***\n",
          static_cast<long long>(delta), severity_idx);
      if (len > 0) {
        AppendToSink(sinks_[sink_idx],
                     std::string(drop_msg, static_cast<size_t>(len)));
      }
    }
  }
}

void LogWriterThread::CheckDropsForQueue(size_t q_idx) {
  for (int s = 0; s < static_cast<int>(kNumSeverities); ++s) {
    CheckDropsForSeverity(q_idx, static_cast<LogSeverity>(s), s);
  }
}

void LogWriterThread::CheckAndReportDrops() {
  for (size_t q_idx = 0; q_idx < queues_.size(); ++q_idx) {
    CheckDropsForQueue(q_idx);
  }
}

void LogWriterThread::AppendToSink(SinkState& sink,
                                   const std::string& formatted) {
  if (sink.total_buffered_bytes + formatted.size() > kMaxSinkBufferedBytes) {
    sink.sink_dropped_count++;
    return;
  }

  if (sink.pending_buffers.empty() ||
      sink.pending_buffers.back().size() + formatted.size() >
          kMaxChunkSizeBytes) {
    sink.pending_buffers.emplace_back();
  }

  sink.pending_buffers.back().append(formatted);
  sink.total_buffered_bytes += formatted.size();
}

void LogWriterThread::ProcessAndRouteEntry(const LogEntry& entry) {
  std::string formatted = entry.ToString();
  for (auto& sink : sinks_) {
    if (AcceptsSeverity(sink, entry.severity)) {
      AppendToSink(sink, formatted);
      break;
    }
  }
}

bool LogWriterThread::DrainQueues() {
  LogEntry entry;
  bool more_work_remaining = false;

  for (auto& queue : queues_) {
    size_t popped_count = 0;
    while (popped_count < kMaxEntriesPerQueuePerCycle && queue->Pop(&entry)) {
      ProcessAndRouteEntry(entry);
      popped_count++;
    }

    if (popped_count == kMaxEntriesPerQueuePerCycle) {
      // Hit quota for this queue; request immediate re-run without sleeping
      more_work_remaining = true;
    }
  }

  return more_work_remaining;
}

bool LogWriterThread::WriteSinkBuffer(SinkState& sink) {
  if (sink.pending_buffers.empty()) {
    return false;
  }

  auto& front_chunk = sink.pending_buffers.front();
  ssize_t written = write(sink.fd, front_chunk.data(), front_chunk.size());

  if (written < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      // Sink congested; keep data buffered and indicate pending work remains
      return true;
    }
    if (errno == EINTR) {
      return true;
    }
    return false;  // Unrecoverable error
  }

  sink.total_buffered_bytes -= static_cast<size_t>(written);

  if (static_cast<size_t>(written) == front_chunk.size()) {
    sink.pending_buffers.pop_front();
  } else {
    front_chunk.erase(0, static_cast<size_t>(written));
  }

  // Return true if more buffers are queued up for this sink
  return !sink.pending_buffers.empty();
}

bool LogWriterThread::FlushBuffers() {
  bool pending_sink_work = false;
  for (auto& sink : sinks_) {
    if (WriteSinkBuffer(sink)) {
      pending_sink_work = true;
    }
  }
  return pending_sink_work;
}

void LogWriterThread::DrainRemainingQueues() {
  LogEntry entry;
  for (auto& queue : queues_) {
    while (queue->Pop(&entry)) {
      ProcessAndRouteEntry(entry);
    }
  }
  CheckAndReportDrops();

  // Non-sleeping final flush sweep
  for (int attempt = 0; attempt < 50; ++attempt) {
    bool all_empty = true;
    for (auto& sink : sinks_) {
      if (WriteSinkBuffer(sink)) {
        all_empty = false;
      }
    }
    if (all_empty) break;
  }
}

void LogWriterThread::RunLoop() {
  int current_sleep_ms = 1;
  constexpr int kMinSleepMs = 1;
  constexpr int kMaxSleepMs = 10;

  while (!stop_notification_.HasBeenNotified()) {
    CheckAndReportDrops();
    bool more_queues = DrainQueues();
    bool more_sinks = FlushBuffers();

    bool did_work = more_queues || more_sinks;

    if (did_work) {
      // Reset backoff and immediately re-run loop without sleeping
      current_sleep_ms = kMinSleepMs;
    } else {
      base::SleepFor(base::Milliseconds(current_sleep_ms));
      if (current_sleep_ms < kMaxSleepMs) {
        ++current_sleep_ms;
      }
    }
  }

  DrainRemainingQueues();
}

}  // namespace base::internal

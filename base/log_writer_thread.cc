#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "base/cpu.h"
#include "base/log_queue.h"
#include "base/log_writer_thread.h"
#include "base/logging_internal.h"
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

std::string GetProcessSpecificLogPath() {
  char buf[1024];
  ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);

  const char* binary_name = "mondale_service";
  if (len != -1) {
    buf[len] = '\0';
    const char* last_slash = std::strrchr(buf, '/');
    if (last_slash != nullptr) {
      binary_name = last_slash + 1;
    } else {
      binary_name = buf;
    }
  }

  return std::string("/var/log/mondale/") + binary_name + "_" +
         std::to_string(getpid()) + ".log";
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

// static
std::function<void()> LogWriterThread::PokeFunction() {
  return []() {
    auto* const instance = LogWriterThread::Instance();
    if (nullptr == instance) return;
    instance->Poke();
  };
}

void LogWriterThread::Stop() {
  stop_notification_.Notify();
  Poke();
}

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
      const int64_t expected_gen =
          poke_generation_.load(std::memory_order_relaxed);
      MutexLock l(&poke_mu_);
      const bool woken_early = poke_mu_.AwaitWithTimeout(
          [this, expected_gen]() {
            return stop_notification_.HasBeenNotified() ||
                   (poke_generation_.load(std::memory_order_relaxed) >
                    expected_gen);
          },
          base::Milliseconds(current_sleep_ms));

      if (woken_early) {
        current_sleep_ms = kMinSleepMs;
      } else {
        if (current_sleep_ms < kMaxSleepMs) {
          ++current_sleep_ms;
        }
      }
    }
  }

  DrainRemainingQueues();
}

// Called by external threads.
void LogWriterThread::Poke() {
  MutexLock l(&poke_mu_);
  poke_generation_.store(poke_generation_.load(std::memory_order_relaxed),
                         std::memory_order_relaxed);
}

void InitializeLoggingSinks(std::vector<std::unique_ptr<LogQueue>> queues) {
  std::vector<SinkState> sinks;
  size_t num_queues = queues.size();

  // 1. Stdout Sink (FD 1): Captures Info and Warning severities
  SinkState stdout_sink;
  stdout_sink.fd = STDOUT_FILENO;
  stdout_sink.severity_mask = (1 << static_cast<int>(LogSeverity::kInfo)) |
                              (1 << static_cast<int>(LogSeverity::kWarning));
  stdout_sink.last_reported_drops.resize(num_queues, {});
  sinks.push_back(std::move(stdout_sink));

  // 2. Stderr Sink (FD 2): Captures Error and Fatal severities
  SinkState stderr_sink;
  stderr_sink.fd = STDERR_FILENO;
  stderr_sink.severity_mask = (1 << static_cast<int>(LogSeverity::kError)) |
                              (1 << static_cast<int>(LogSeverity::kFatal));
  stderr_sink.last_reported_drops.resize(num_queues, {});
  sinks.push_back(std::move(stderr_sink));

  // 3. File Sink: Captures all severities into a process-specific file under
  // /var/log/mondale
  std::string log_path = GetProcessSpecificLogPath();
  int file_fd = open(log_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);

  if (file_fd >= 0) {
    internal::SinkState file_sink;
    file_sink.fd = file_fd;
    file_sink.severity_mask = 0xFF;  // Accept all severities
    file_sink.last_reported_drops.resize(num_queues, {});
    sinks.push_back(std::move(file_sink));
  }

  // Start the background writer thread singleton
  LogWriterThread::Init(std::move(queues), std::move(sinks));
}

void InitializeLoggingThread() {
  const int num_queues = std::min<int>(16, NumCpus());
  std::vector<std::unique_ptr<LogQueue>> qs;
  qs.reserve(num_queues);
  for (int i = 0; i < num_queues; ++i) {
    qs.push_back(std::make_unique<LogQueue>(LogWriterThread::PokeFunction()));
  }
  InitializeLoggingSinks(std::move(qs));
}

}  // namespace base::internal

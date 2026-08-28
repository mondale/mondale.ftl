#ifndef BASE_LOG_QUEUE_H_
#define BASE_LOG_QUEUE_H_

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <utility>

#include "base/logging_internal.h"
#include "base/mpsc_queue.h"

namespace base {

class LogQueue {
 public:
  static constexpr size_t kCapacity = 16384;
  static constexpr int64_t kThreshold = 8192;

  using PokeFn = std::function<void()>;

  explicit LogQueue(PokeFn poke_fn) : poke_fn_(std::move(poke_fn)) {}

  ~LogQueue() = default;

  LogQueue(const LogQueue&) = delete;
  LogQueue& operator=(const LogQueue&) = delete;

  // Enqueues a log entry.
  // If capacity is exceeded, increments the drop counter for entry's severity
  // and pokes. Triggers poke_fn if post-enqueue occupancy exceeds kThreshold.
  //
  // Returns true when successfully pushed.
  bool Push(internal::LogEntry entry) {
    int64_t occupancy = 0;
    internal::LogSeverity severity = entry.severity;

    if (!queue_.TryEnqueue(std::move(entry), &occupancy)) {
      size_t idx = static_cast<size_t>(severity);
      if (idx < static_cast<size_t>(internal::LogSeverity::kFatal)) {
        dropped_counts_[idx].fetch_add(1, std::memory_order_relaxed);
      }
      poke_fn_();
      return false;
    }

    if (occupancy > kThreshold) {
      poke_fn_();
    }
    return true;
  }

  // Dequeues a single log entry. Single consumer thread only.
  [[nodiscard]] bool Pop(internal::LogEntry* out) {
    return queue_.Dequeue(out);
  }

  // Returns dropped count for a specific severity level.
  [[nodiscard]] int64_t dropped_count(internal::LogSeverity severity) const {
    size_t idx = static_cast<size_t>(severity);
    if (idx >= static_cast<size_t>(internal::LogSeverity::kFatal)) {
      return 0;
    }
    return dropped_counts_[idx].load(std::memory_order_relaxed);
  }

  static constexpr size_t capacity() { return kCapacity; }

 private:
  MpscQueue<internal::LogEntry, kCapacity> queue_;
  PokeFn poke_fn_;

  // Per-severity drop metrics aligned to avoid false sharing
  alignas(64) std::array<std::atomic<int64_t>,
                         static_cast<size_t>(internal::LogSeverity::kFatal) +
                             1> dropped_counts_{};
};

}  // namespace base

#endif  // #ifndef BASE_LOG_QUEUE_H_

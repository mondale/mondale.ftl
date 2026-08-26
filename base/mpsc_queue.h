#ifndef BASE_MPSC_QUEUE_H_
#define BASE_MPSC_QUEUE_H_

#include <array>
#include <atomic>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

namespace base {

template <typename T>
concept MpscQueueElement = std::movable<T> && std::is_nothrow_destructible_v<T>;

// Generic Multi-Producer, Single-Consumer (MPSC) bounded lock-free ring buffer.
// Zero heap allocations during Enqueue/Dequeue operations.
template <MpscQueueElement T, size_t Capacity>
class MpscQueue {
 public:
  static_assert(Capacity > 0, "MpscQueue Capacity must be greater than 0");

  MpscQueue() = default;

  ~MpscQueue() {
    T dummy;
    while (Dequeue(&dummy)) {
      // Drain remaining elements so their destructors run.
    }
  }

  MpscQueue(const MpscQueue&) = delete;
  MpscQueue& operator=(const MpscQueue&) = delete;

  // Enqueues an item. Returns false if the queue is full.
  // Thread-safe for multiple concurrent producers.
  [[nodiscard]] bool TryEnqueue(T item, int64_t* occupancy = nullptr) {
    int64_t current_tail = tail_.load(std::memory_order_relaxed);
    int64_t current_head = 0;

    while (true) {
      current_head = head_.load(std::memory_order_relaxed);
      if (current_tail - current_head >= static_cast<int64_t>(Capacity)) {
        return false;
      }

      // Only advance tail_ if there is capacity and no other producer beat us
      // to this ticket.
      if (tail_.compare_exchange_weak(current_tail, current_tail + 1,
                                      std::memory_order_relaxed,
                                      std::memory_order_relaxed)) {
        break;
      }
    }

    size_t idx = static_cast<size_t>(current_tail % Capacity);
    Cell& cell = ring_[idx];

    ::new (static_cast<void*>(cell.storage)) T(std::move(item));
    cell.ready.store(true, std::memory_order_release);

    if (occupancy != nullptr) {
      *occupancy = (current_tail + 1) - current_head;
    }
    return true;
  }

  // Dequeues an item into `out`. Returns false if queue is empty or entry is
  // not ready. Thread-compatible for a SINGLE background consumer thread.
  [[nodiscard]] bool Dequeue(T* out) {
    int64_t current_head = head_.load(std::memory_order_relaxed);
    size_t idx = static_cast<size_t>(current_head % Capacity);
    Cell& cell = ring_[idx];

    if (!cell.ready.load(std::memory_order_acquire)) {
      return false;
    }

    T* val_ptr = std::launder(reinterpret_cast<T*>(cell.storage));
    *out = std::move(*val_ptr);
    val_ptr->~T();

    cell.ready.store(false, std::memory_order_release);
    head_.store(current_head + 1, std::memory_order_relaxed);
    return true;
  }

  static constexpr size_t capacity() { return Capacity; }

 private:
  struct Cell {
    alignas(T) std::byte storage[sizeof(T)];
    std::atomic<bool> ready{false};
  };

  alignas(64) std::atomic<int64_t> tail_{0};
  alignas(64) std::atomic<int64_t> head_{0};
  alignas(64) std::array<Cell, Capacity> ring_;
};

}  // namespace base

#endif  // #ifndef BASE_MPSC_QUEUE_H_

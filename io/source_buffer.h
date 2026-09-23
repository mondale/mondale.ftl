#ifndef IO_SOURCE_BUFFER_H_
#define IO_SOURCE_BUFFER_H_

#include "core/vocabulary.h"

namespace io {

class SourceBuffer final {
 public:
  SourceBuffer() = delete;
  SourceBuffer(
      const char* p, size_t n,
      std::move_only_function<void()> c = []() noexcept {})
      : memory_(p), remain_(n), cleanup_(std::move(c)) {}

  ~SourceBuffer() { cleanup_(); }

  SourceBuffer(const SourceBuffer&) = delete;
  SourceBuffer& operator=(const SourceBuffer&) = delete;

  SourceBuffer(SourceBuffer&& other) noexcept
      : memory_(other.memory_),
        remain_(other.remain_),
        cleanup_(std::move(other.cleanup_)) {
    other.memory_ = nullptr;
    other.remain_ = 0;
    other.cleanup_ = []() noexcept {};
  }

  SourceBuffer& operator=(SourceBuffer&& other) noexcept {
    if (this != &other) {
      memory_ = other.memory_;
      remain_ = other.remain_;
      cleanup_();
      cleanup_ = std::move(other.cleanup_);
      other.memory_ = nullptr;
      other.remain_ = 0;
      other.cleanup_ = []() noexcept {};
    }
    return *this;
  }

  const char* data() const { return memory_; }
  size_t size() const { return remain_; }

  void Consume(size_t amount) {
    DCHECK_GE(remain_, amount);
    memory_ += amount;
    remain_ -= amount;
  }

 protected:
  const char* memory_;
  size_t remain_;
  std::move_only_function<void()> cleanup_;
};

}  // namespace io

#endif  // #ifndef IO_SOURCE_BUFFER_H_

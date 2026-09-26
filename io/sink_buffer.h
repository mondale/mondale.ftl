#ifndef IO_SINK_BUFFER_H_
#define IO_SINK_BUFFER_H_

#include "core/vocabulary.h"

namespace io {

class SinkBuffer final {
 public:
  SinkBuffer() = delete;
  SinkBuffer(
      char* p, size_t n,
      std::move_only_function<void(size_t)> c = [](size_t) noexcept {})
      : ptr_(p), n_(n), written_(0), cleanup_(std::move(c)) {}
  ~SinkBuffer() { cleanup_(written_); }

  SinkBuffer(const SinkBuffer&) = delete;
  SinkBuffer& operator=(const SinkBuffer&) = delete;

  SinkBuffer(SinkBuffer&& other) noexcept
      : ptr_(other.ptr_),
        n_(other.n_),
        written_(other.written_),
        cleanup_(std::move(other.cleanup_)) {
    other.ptr_ = nullptr;
    other.cleanup_ = [](size_t) noexcept {};
  }

  SinkBuffer& operator=(SinkBuffer&& other) noexcept {
    if (this != &other) {
      ptr_ = other.ptr_;
      written_ = other.written_;
      cleanup_(other.written_);
      cleanup_ = std::move(other.cleanup_);
      other.ptr_ = nullptr;
      other.cleanup_ = [](size_t) noexcept {};
    }
    return *this;
  }

  char* data() { return ptr_; }
  size_t size() const { return (n_ - written_); }

  bool Advance(size_t amount) {
    DCHECK_LE(amount, size());
    written_ += amount;
    ptr_ += amount;
    return 0 == size();
  }

 private:
  char* ptr_;
  const size_t n_;
  size_t written_;
  std::move_only_function<void(size_t)> cleanup_;
};

}  // namespace io

#endif  // #ifndef IO_SINK_BUFFER_H_

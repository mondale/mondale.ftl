#ifndef IO_TEST_HANDLERS_H_
#define IO_TEST_HANDLERS_H_

#include <atomic>
#include <cstdint>
#include <vector>

#include "base/logging.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace io::testing {

struct Stuff final {
  int64_t Reads() const { return read_upcalls.load(std::memory_order_acquire); }
  int64_t Writes() const {
    return write_upcalls.load(std::memory_order_acquire);
  }
  int64_t BytesRead() const {
    return bytes_read.load(std::memory_order_acquire);
  }
  int64_t BytesWritten() const {
    return bytes_written.load(std::memory_order_acquire);
  }

  std::atomic<int> refs{0};
  std::atomic<int64_t> read_upcalls{0};
  std::atomic<int64_t> write_upcalls{0};
  std::atomic<int64_t> bytes_read{0};
  std::atomic<int64_t> bytes_written{0};
};

class HandlerBase {
 public:
  explicit HandlerBase(Stuff* s) : s_(s) { s_->refs++; }
  ~HandlerBase() {
    s_->refs--;
    CHECK_GE(s_->refs.load(std::memory_order_acquire), 0);
  }

  void HandledWrite(size_t bytes) {
    s_->write_upcalls++;
    s_->bytes_written += bytes;
  }

  void HandledRead(size_t bytes) {
    s_->read_upcalls++;
    s_->bytes_read += bytes;
  }

 private:
  Stuff* const s_;
};

// Always willing to read, down for a good time.
class EagerSwallowHandler final : public HandlerBase, public IoHandler {
 public:
  explicit EagerSwallowHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  virtual ~EagerSwallowHandler() {}
  static constexpr size_t kSwallowSize = 64;

  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override;

 private:
  char buf_[kSwallowSize];
};

// Always has something to write.
class GarbageFountainHandler final : public HandlerBase, public IoHandler {
 public:
  explicit GarbageFountainHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override;
};

// Hates all file descriptors and just wants them to die.
class ClosingHandler final : public HandlerBase, public IoHandler {
 public:
  explicit ClosingHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override;
};

// Merrily echoes whatever it reads, but has a small internal buffer.
class EchoingHandler final : public HandlerBase, public IoHandler {
 public:
  explicit EchoingHandler(Stuff* s) : HandlerBase(s), IoHandler() {}
  static constexpr size_t kSize = 64;
  ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                               const core::FileDescriptor& fd) override;
  ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                const core::FileDescriptor& fd) override;

 private:
  std::vector<char> buffer_;
};

}  // namespace io::testing

#endif  // #ifndef IO_TEST_HANDLERS_H_

#ifndef IO_IO_HANDLER_H_
#define IO_IO_HANDLER_H_

#include <atomic>
#include <list>
#include <memory>

#include "core/file_descriptor.h"
#include "core/inlined_vector.h"
#include "core/vocabulary.h"

namespace io {

HANDLE_TYPE(FdHandle, int64_t);

class Context;

class IoHandler {
 public:
  enum class Outcome {
    kFdEagain,  // FD had an EAGAIN
    kYield,     // Please call me again
    kSuspend,   // Please don't call until I ask.
    kClose,     // Please close the FD and stop calling me.
  };

  virtual ResultOr<Outcome> HandleRead(Context* c, FdHandle h,
                                       const core::FileDescriptor& fd) = 0;
  virtual ResultOr<Outcome> HandleWrite(Context* c, FdHandle h,
                                        const core::FileDescriptor& fd) = 0;
  virtual Result HandleIdle(Context* c, FdHandle h,
                            const core::FileDescriptor& fd) {
    return Result(Code::kDeadline);
  }

  // Helpers for subclasses to invoke.
  ResultOr<size_t> NonBlockingRead(IoHandler::Outcome* outcome,
                                   const core::FileDescriptor& fd, char* buf,
                                   size_t count) const;
  ResultOr<size_t> NonBlockingWrite(IoHandler::Outcome* outcome,
                                    const core::FileDescriptor& fd,
                                    const char* buf, size_t count) const;

  Duration idle_threshold() const { return idle_threshold_; }
  void set_idle_threshold(Duration d) { idle_threshold_ = d; }
  Duration idle_threshold_ = base::Minutes(5);

  // Subclasses, ignore all below.
  static constexpr int kNoAffinity = INT_MAX;
  friend class Epoller;
  friend class Silo;

  void SetAffinity(int a) { affinity_.store(a, std::memory_order_release); }
  int GetAffinity() const { return affinity_.load(std::memory_order_acquire); }

  std::atomic<int> affinity_{kNoAffinity};
  std::list<FdHandle> handles_;
};

std::string ToString(IoHandler::Outcome o);
std::ostream& operator<<(std::ostream& out, IoHandler::Outcome o);

namespace internal {
struct HFDs {
  std::shared_ptr<IoHandler> h;
  core::InlinedVector<core::FileDescriptor, 2> fds;
};
}  // namespace internal

}  // namespace io

#endif  // #ifndef IO_IO_HANDLER_H_

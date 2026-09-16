#ifndef IO_IO_HANDLER_H_
#define IO_IO_HANDLER_H_

#include <atomic>
#include <memory>

#include "core/file_descriptor.h"
#include "core/vocabulary.h"

namespace io {

HANDLE_TYPE(FdHandle, int64_t);

class Epoller;
class PollingContext;

class IoHandler {
 public:
  enum class Outcome {
    kFdEagain,  // FD had an EAGAIN
    kYield,     // Please call me again
    kSuspend,   // Please don't call until I ask.
    kClose,     // Please close the FD and stop calling me.
  };

  virtual Outcome HandleRead(PollingContext* c, FdHandle h,
                             const core::FileDescriptor& fd) = 0;
  virtual Outcome HandleWrite(PollingContext* c, FdHandle h,
                              const core::FileDescriptor& fd) = 0;

 private:
  static constexpr int kNoAffinity = INT_MAX;
  friend class Epoller;

  void SetAffinity(int a) { affinity_.store(a, std::memory_order_release); }
  int GetAffinity() const { return affinity_.load(std::memory_order_acquire); }

  std::atomic<int> affinity_{kNoAffinity};
};

namespace internal {
struct HFDPair {
  std::shared_ptr<IoHandler> h;
  core::FileDescriptor fd;
};
}  // namespace internal

}  // namespace io

#endif  // #ifndef IO_IO_HANDLER_H_

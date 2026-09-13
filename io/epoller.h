#ifndef IO_EPOLLER_H_
#define IO_EPOLLER_H_

#include "core/file_descriptor.h"
#include "core/vocabulary.h"

namespace io {

class Epoller final {
 public:
  HANDLE_TYPE(FdHandle, int64_t);

  Epoller();
  ~Epoller();

  enum class Action {
    kFdEagain,  // FD had an EAGAIN
    kYield,     // Please call me again
    kSuspend,   // Please don't call until I ask.
    kClose,     // Please close the FD and stop calling me.
  };

  class Handler {
   public:
    virtual Action HandleRead(Epoller* e, FdHandle h,
                              const core::FileDescriptor& fd) = 0;
    virtual Action HandleWrite(Epoller* e, FdHandle h,
                               const core::FileDescriptor& fd) = 0;
  };

  // Request a call to HandleRead for the Handler associated with h.
  void RequestRead(FdHandle h);

  // Request a call to HandleWrite for the Handler associated with h.
  void RequestWrite(FdHandle h);

  // Request to run 'fn' sometime in the near future.
  void Run(std::move_only_function<void()> fn);

  // Register a new file descriptor with the epoll set.
  ResultOr<FdHandle> Register(core::FileDescriptor fd,
                              std::shared_ptr<Handler> h);

 private:
  class Impl;
  Impl* const impl_;
};

}  // namespace io

#endif  // #ifndef IO_EPOLLER_H_

#ifndef IO_EPOLLER_H_
#define IO_EPOLLER_H_

#include "core/file_descriptor.h"
#include "core/vocabulary.h"

namespace io {

class Epoller final {
 public:
  enum class Action {
    kFdEagain,  // FD had an EAGAIN
    kYield,     // Please call me again
    kSuspend,   // Please don't call until I ask.
    kClose,     // Please close the FD and stop calling me.
  };

  class Control {
   public:
    virtual void RequestWrite() = 0;
    virtual void RequestRead() = 0;
  };

  class Handler {
   public:
    virtual Action HandleRead(const core::FileDescriptor& fd) = 0;
    virtual Action HandleWrite(const core::FileDescriptor& fd) = 0;
  };

  ResultOr<std::unique_ptr<Control>> Register(core::FileDescriptor fd,
                                              std::shared_ptr<Handler> h);

 private:
};

}  // namespace io

#endif  // #ifndef IO_EPOLLER_H_

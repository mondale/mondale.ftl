#ifndef IO_EPOLLER_H_
#define IO_EPOLLER_H_

#include <vector>

#include "core/file_descriptor.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace io {

class Epoller final {
 public:
  Epoller();
  ~Epoller();

  static ResultOr<std::unique_ptr<Epoller>> Build(int silos);

  // Register a new file descriptor with the epoll set. Ownership of fd
  // transfers to the Epoller. The mapping from h->fd is one to many.
  Result Register(std::shared_ptr<IoHandler> h, core::FileDescriptor&& fd);
  Result SetNonBlockingAndRegister(std::shared_ptr<IoHandler> h,
                                   core::FileDescriptor&& fd);

 private:
  int SelectSilo();

  void Route(internal::HFDs&& i);
  void RouteTo(int silo, internal::HFDs&& i);
  void Peek(std::vector<int>* utils);

  Notification exiting_;
  struct PerThread {
    Mutex mu;
    std::list<internal::HFDs> inbound GUARDED_BY(mu);
    std::atomic<int> utilization{0};  // [0, 100].
    core::FileDescriptor event_fd;
    std::unique_ptr<Thread> thread;
  };
  std::vector<std::unique_ptr<PerThread>> threads_;
};

}  // namespace io

#endif  // #ifndef IO_EPOLLER_H_

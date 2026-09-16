#ifndef IO_EPOLLER_H_
#define IO_EPOLLER_H_

#include <vector>

#include "core/file_descriptor.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace io {

// PollingContext is a bag holder and API gateway for using inside the
// Epoller's upcalls.
class Epoller;
class PollingContext final {
 public:
  explicit PollingContext(Epoller* e) : e_(e) {}

  void RequestRead(PollingContext* c, FdHandle h);
  void RequestWrite(PollingContext* c, FdHandle h);
  void Run(PollingContext* c, std::move_only_function<void()> fn);

 private:
  Epoller* const e_;
};

class Epoller final {
 public:
  Epoller();
  ~Epoller();

  static ResultOr<std::unique_ptr<Epoller>> Build(int silos);

  // Register a new file descriptor with the epoll set. Ownership of fd
  // transfers to the Epoller. The mapping from h->fd is one to many.
  Result Register(std::shared_ptr<IoHandler> h, core::FileDescriptor&& fd);

 private:
  friend class PollingContext;

  // Request a call to HandleRead for the IoHandler associated with h.
  void RequestRead(PollingContext* c, FdHandle h);

  // Request a call to HandleWrite for the IoHandler associated with h.
  void RequestWrite(PollingContext* c, FdHandle h);

  // Request to run 'fn' sometime in the near future.
  void Run(PollingContext* c, std::move_only_function<void()> fn);

  int SelectSilo();

  void Route(internal::HFDPair&& i);
  void RouteTo(int silo, internal::HFDPair&& i);

  Notification exiting_;
  struct PerThread {
    Mutex mu;
    std::list<internal::HFDPair> inbound GUARDED_BY(mu);
    std::atomic<int> utilization{0};  // [0, 100].
    std::unique_ptr<Thread> thread;
  };
  std::vector<std::unique_ptr<PerThread>> threads_;
};

inline void PollingContext::RequestRead(PollingContext* c, FdHandle h) {
  e_->RequestRead(c, h);
}
inline void PollingContext::RequestWrite(PollingContext* c, FdHandle h) {
  e_->RequestWrite(c, h);
}
inline void PollingContext::Run(PollingContext* c,
                                std::move_only_function<void()> fn) {
  e_->Run(c, std::move(fn));
}

}  // namespace io

#endif  // #ifndef IO_EPOLLER_H_

#ifndef IO_EPOLLER_H_
#define IO_EPOLLER_H_

#include <vector>

#include "core/file_descriptor.h"
#include "core/vocabulary.h"

namespace io {

HANDLE_TYPE(FdHandle, int64_t);

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

  enum class Outcome {
    kFdEagain,  // FD had an EAGAIN
    kYield,     // Please call me again
    kSuspend,   // Please don't call until I ask.
    kClose,     // Please close the FD and stop calling me.
  };

  class Handler {
   public:
    virtual Outcome HandleRead(PollingContext* c, FdHandle h,
                               const core::FileDescriptor& fd) = 0;
    virtual Outcome HandleWrite(PollingContext* c, FdHandle h,
                                const core::FileDescriptor& fd) = 0;

   private:
    static constexpr int kNoAffinity = INT_MAX;
    friend class Epoller;

    void SetAffinity(int a) { affinity_.store(a, std::memory_order_release); }
    int GetAffinity() const {
      return affinity_.load(std::memory_order_acquire);
    }

    std::atomic<int> affinity_{kNoAffinity};
  };

  // Register a new file descriptor with the epoll set. Ownership of fd
  // transfers to the Epoller. The mapping from h->fd is one to many.
  Result Register(std::shared_ptr<Handler> h, core::FileDescriptor&& fd);

 private:
  friend class PollingContext;

  // Request a call to HandleRead for the Handler associated with h.
  void RequestRead(PollingContext* c, FdHandle h);

  // Request a call to HandleWrite for the Handler associated with h.
  void RequestWrite(PollingContext* c, FdHandle h);

  // Request to run 'fn' sometime in the near future.
  void Run(PollingContext* c, std::move_only_function<void()> fn);

  int SelectSilo();

  struct Inbound {
    std::shared_ptr<Handler> h;
    core::FileDescriptor fd;
  };
  void Route(Inbound&& i);
  void RouteTo(int silo, Inbound&& i);

  Notification exiting_;
  struct PerThread {
    Mutex mu;
    std::list<Inbound> inbound GUARDED_BY(mu);
    std::atomic<int> utilization{0};  // [0, 100].
    std::unique_ptr<Thread> thread;
  };
  std::vector<PerThread> threads_;
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

#ifndef IO_SILO_H_
#define IO_SILO_H_

#include <atomic>
#include <list>
#include <vector>

#include "core/handle_table.h"
#include "core/intrusive_list.h"
#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace io {

class Silo final {
 public:
  static constexpr int kMaxFds = 1024 * 1014;

  using InList = std::list<internal::HFDs>;
  using ShedFn = std::function<void(internal::HFDs&&)>;
  using PeekFn = std::function<void(std::vector<int>*)>;

  Silo(int id, const core::FileDescriptor* efd, Notification* exiting,
       Mutex* inbound_mu, InList* inbound, std::atomic<int>* util, ShedFn sf,
       PeekFn pf);

  // Called via Context.
  void RequestRead(FdHandle h);
  void RequestWrite(FdHandle h);
  void Run(FdHandle h, std::move_only_function<void(Context*)> fn);

  void ThreadMain();

 private:
  struct PerFd;

  Result ThreadMain2();

  void GetInList(InList* swapee) LOCKS_EXCLUDED(inbound_mu_);
  Result RunAdmission(Context* c) LOCKS_EXCLUDED(inbound_mu_);
  void ConsiderLoadShedding(int util);
  Result Add(Context* c, internal::HFDs&& i);
  ResultOr<FdHandle> Add(Context* c, std::shared_ptr<IoHandler> h,
                         core::FileDescriptor fd);
  Result Remove(PerFd* perfd);
  bool ActivationsEmpty() const;
  void RunActives(Context* c);
  void RunRunners(Context* c);
  void RunShedders();

  const int id_;
  Mutex* const inbound_mu_;
  InList* const inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int>* const util_;
  Notification* const exiting_;
  const core::FileDescriptor* const event_fd_;
  ShedFn shed_;
  bool impending_shed_ = false;

  struct ActiveIdle {};
  struct Shared {};

  struct PerFd : public core::IntrusiveListHook<ActiveIdle>,
                 public core::IntrusiveListHook<Shared> {
    std::shared_ptr<IoHandler> handler;
    core::FileDescriptor fd;
    FdHandle handle;
    base::MonotonicTime last_activation;
    bool wants_read = false;
    bool wants_write = false;
    bool squelch_reads = false;
    bool squelch_writes = false;
    std::list<std::move_only_function<void(Context*)>> fns;
  };

  using HTable = core::HandleTable<PerFd, kMaxFds>;
  HTable::Handle Coerce(FdHandle h) const { return HTable::Handle(h.value()); }
  FdHandle Coerce(HTable::Handle h) const { return FdHandle(h.value()); }
  void Expunge(PerFd* perfd);

  HTable ht_;
  core::FileDescriptor efd_;
  core::IntrusiveList<PerFd, ActiveIdle> active_;
  core::IntrusiveList<PerFd, Shared> runners_;
  core::IntrusiveList<PerFd, Shared> closers_;
  core::IntrusiveList<PerFd, Shared> shedders_;
  FdHandle current_ = FdHandle::kInvalid;

  PeekFn peek_;
  std::vector<int> utils_;
};

}  // namespace io

#endif  // #ifndef IO_SILO_H_

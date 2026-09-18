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

  using InList = std::list<internal::HFDPair>;
  using ShedFn = std::function<void(internal::HFDPair&&)>;
  using PeekFn = std::function<void(std::vector<int>*)>;

  Silo(int id, Notification* exiting, Mutex* inbound_mu, InList* inbound,
       std::atomic<int>* util, ShedFn sf, PeekFn pf);

  // Called via Context.
  void RequestRead(FdHandle h);
  void RequestWrite(FdHandle h);
  void Run(FdHandle h, std::move_only_function<void()> fn);

  void ThreadMain();

 private:
  struct PerFd;

  Result ThreadMain2();

  void GetInList(InList* swapee) LOCKS_EXCLUDED(inbound_mu_);
  Result RunAdmission() LOCKS_EXCLUDED(inbound_mu_);
  void ConsiderLoadShedding(int util);
  Result Add(internal::HFDPair&& i);
  Result Remove(PerFd* perfd);
  bool ActivationsEmpty() const;
  void RunReaders(Context* c);
  void RunWriters(Context* c);
  void RunRunners();

  const int id_;
  Mutex* const inbound_mu_;
  InList* const inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int>* const util_;
  Notification* const exiting_;
  ShedFn shed_;
  bool impending_shed_ = false;

  struct Readers {};
  struct Writers {};
  struct Runners {};

  struct PerFd : public core::IntrusiveListHook<Readers>,
                 public core::IntrusiveListHook<Writers>,
                 public core::IntrusiveListHook<Runners> {
    std::shared_ptr<IoHandler> handler;
    core::FileDescriptor fd;
    FdHandle handle;
    std::list<std::move_only_function<void()>> fns;
  };

  using HTable = core::HandleTable<PerFd, kMaxFds>;
  HTable::Handle Coerce(FdHandle h) const { return HTable::Handle(h.value()); }
  FdHandle Coerce(HTable::Handle h) const { return FdHandle(h.value()); }
  void Expunge(PerFd* perfd);

  HTable ht_;
  core::FileDescriptor efd_;
  core::IntrusiveList<PerFd, Readers> readers_;
  core::IntrusiveList<PerFd, Writers> writers_;
  core::IntrusiveList<PerFd, Runners> runners_;
  core::IntrusiveList<PerFd, Runners> closers_;
  FdHandle current_ = FdHandle::kInvalid;

  PeekFn peek_;
  std::vector<int> utils_;
};

}  // namespace io

#endif  // #ifndef IO_SILO_H_

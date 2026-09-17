#ifndef IO_SILO_H_
#define IO_SILO_H_

#include <atomic>

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

  Silo(int id, Notification* exiting, Mutex* inbound_mu, InList* inbound,
       std::atomic<int>* util, ShedFn sf);

  void ThreadMain();

 private:
  struct PerFd;

  Result ThreadMain2();

  void GetInList(InList* swapee) LOCKS_EXCLUDED(inbound_mu_);
  Result RunAdmission() LOCKS_EXCLUDED(inbound_mu_);
  Result Add(internal::HFDPair&& i);
  Result Remove(PerFd* perfd);

  const int id_;
  Mutex* const inbound_mu_;
  InList* const inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int>* const util_;
  Notification* const exiting_;
  ShedFn shed_;

  struct Readers {};
  struct Writers {};
  struct Runners {};

  struct PerFd : public core::IntrusiveListHook<Readers>,
                 public core::IntrusiveListHook<Writers>,
                 public core::IntrusiveListHook<Runners> {
    std::shared_ptr<IoHandler> handler;
    core::FileDescriptor fd;
    FdHandle handle;
  };
  using HTable = core::HandleTable<PerFd, kMaxFds>;
  HTable::Handle Coerce(FdHandle h) const { return HTable::Handle(h.value()); }
  FdHandle Coerce(HTable::Handle h) const { return FdHandle(h.value()); }
  HTable ht_;
  core::FileDescriptor efd_;
  core::IntrusiveList<PerFd, Readers> readers_;
  core::IntrusiveList<PerFd, Writers> writers_;
  core::IntrusiveList<PerFd, Runners> runners_;
};

}  // namespace io

#endif  // #ifndef IO_SILO_H_

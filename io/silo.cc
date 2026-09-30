#include <array>

#include "core/handle_table.h"
#include "core/syscalls.h"
#include "io/context.h"
#include "io/silo.h"
#include "io/utilization_estimator.h"

namespace io {
namespace {

class SiloContext final : public Context {
 public:
  SiloContext(Silo* s, base::MonotonicTime now) : Context(now), s_(s) {}

  void RequestRead(FdHandle h) override { s_->RequestRead(h); }
  void RequestWrite(FdHandle h) override { s_->RequestWrite(h); }
  void Run(FdHandle h, std::move_only_function<void()> fn) override {
    s_->Run(h, std::move(fn));
  }
  ResultOr<FdHandle> Add(std::shared_ptr<IoHandler> h,
                         core::FileDescriptor fd) override {
    return s_->Add(std::move(h), std::move(fd));
  }
  void Eject(FdHandle h) override { s_->Eject(h); }

 private:
  Silo* const s_;
};

}  // namespace

Silo::Silo(int id, const core::FileDescriptor* efd, Notification* exiting,
           Mutex* inbound_mu, InList* inbound, std::atomic<int>* util,
           ShedFn sf, PeekFn pf)
    : id_(id),
      inbound_mu_(inbound_mu),
      inbound_(inbound),
      util_(util),
      exiting_(exiting),
      event_fd_(efd),
      shed_(std::move(sf)),
      peek_(std::move(pf)) {}

void Silo::ThreadMain() {
  auto ret = ThreadMain2();
  if (!ret.IsOk()) {
    Log(ERROR) << "Thread silo exiting with error: " << ret.ToString();
  }
}

Result Silo::ThreadMain2() {
  UtilizationEstimator ue;
  TRY_ASSIGN(efd_, core::syscalls::EpollCreate1(EPOLL_CLOEXEC));

  // Add the event FD to the epoll set.
  constexpr uint64_t kIsPollFd = UINT64_MAX;
  {
    struct epoll_event e;
    e.events = EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLIN | EPOLLOUT | EPOLLET;
    e.data.u64 = kIsPollFd;
    TRY(core::syscalls::EpollCtl(efd_, EPOLL_CTL_ADD, *event_fd_, &e));
  }

  struct timespec timeout = Milliseconds(10).ToTimespec();
  constexpr int kMaxEvents = 64;
  alignas(64) std::array<struct epoll_event, kMaxEvents> events;

  while (!exiting_->HasBeenNotified()) {
    const auto idle_start = MonotonicTime::Now();
    // Invoke Epoll.
    TRY_ASSIGN(const auto ready_count,
               core::syscalls::EpollPwait2(efd_, events.data(), kMaxEvents,
                                           &timeout, /*sigmask*/ nullptr));
    const auto busy_start = MonotonicTime::Now();
    SiloContext context(this, busy_start);
    Context::SetCurrent(&context);

    // Load shedding metadata.
    const int shed_target = impending_shed_ ? (ready_count + 1) / 2 : 0;
    impending_shed_ = false;
    int shed_count = 0;

    // Convert epoll events to list activations.
    for (int i = 0; i < ready_count; ++i) {
      const auto e = events[i].events;
      const uint64_t datum = events[i].data.u64;

      // Check for an eventfd edge -> very special case.
      if (kIsPollFd == datum) {
        static_cast<void>(core::syscalls::EventFdRead(*event_fd_));
        continue;
      }

      const auto h = FdHandle(static_cast<int64_t>(datum));

      // Lookup handle and get the per-FD object.
      TRY_ASSIGN(auto* perfd, ht_.Lookup(Coerce(h)));

      // Move to error list?
      if ((e & EPOLLERR) || (e & EPOLLHUP) || (e & EPOLLRDHUP)) {
        // Expunge the per-FD object from all lists.
        Expunge(perfd);

        // Place the per-FD object in the closure list.
        closers_.PushBack(perfd);

        // Do not place in any additional lists.
        continue;
      }

      // Possibly shed. Prefer to shed active FDs so the shedding is more
      // effective.
      if (shed_count < shed_target) {
        Expunge(perfd);
        shedders_.PushBack(perfd);
        shed_count++;
        continue;
      }

      // Add to read list if not already there?
      if ((e & EPOLLIN) && !perfd->squelch_reads) {
        perfd->wants_read = true;
      }

      // Add to write list if not already there?
      if ((e & EPOLLOUT) && !perfd->squelch_writes) {
        perfd->wants_write = true;
      }

      if (perfd->wants_read || perfd->wants_write) {
        core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
        active_.PushBack(perfd);
      }
    }

    // Shed any chosed FDs.
    RunShedders();

    // Until they are all empty...
    while (!ActivationsEmpty()) {
      RunActives();
      RunRunners();
    }

    // Eject Fds that have been removed.
    RunEjecters();

    // Scan for idle descriptors that need to be closed.
    RunIdlers();

    // Close doomed file handles.
    TRY(RunClosers());

    // Run the inbound admission.
    TRY(RunAdmission());

    // Run load shedding. Deliberatly use last cycle's estimate.
    ConsiderLoadShedding(ue.Estimate());

    // End loop and report utilization.
    const auto loop_end = MonotonicTime::Now();
    const auto busy_duration = loop_end - busy_start;
    const auto idle_duration = busy_start - idle_start;
    ue.Sample(busy_duration.ToNanoseconds(), idle_duration.ToNanoseconds());
    util_->store(ue.Estimate(), std::memory_order_release);
  }
  return Result::Ok();
}

void Silo::GetInList(InList* swapee) {
  MutexLock l(inbound_mu_);
  swapee->swap(*inbound_);
}

void Silo::ConsiderLoadShedding(int util) {
  // Do not shed if lightly loaded.
  impending_shed_ = false;
  const int kThresh = 25;
  if (util <= kThresh) return;

  // Do not shed if no other silo is significantly less loaded.
  peek_(&utils_);
  for (int i = 0; i < utils_.size(); ++i) {
    if ((utils_[i] + kThresh) < util) {
      // Imbalanced.
      impending_shed_ = true;
      return;
    }
  }
}

Result Silo::RunAdmission() {
  InList in;
  GetInList(&in);
  for (auto& i : in) {
    // Check for an affinity race condition -- route non-matching affinity
    // elsewhere.
    if (i.h->GetAffinity() != id_) {
      shed_(std::move(i));
      continue;
    }

    // This is either a request to run a function on this thread, or it's an
    // inbound handle.
    if (i.fds.empty()) {
      // Request to run function.
      RunWithAffinity(i.h, std::move(i.fn));
    } else {
      TRY(Add(std::move(i)));
    }
  }
  return Result::Ok();
}

void Silo::RunWithAffinity(std::shared_ptr<IoHandler> h,
                           std::move_only_function<void()> fn) {
  if (h->handles_.empty()) {
    Log(WARNING) << "Abandoning affinitized function due to lack of FdHandles.";
    return;
  }
  current_ = h->handles_.front();
  fn();
  current_ = FdHandle::kInvalid;
}

Result Silo::Add(internal::HFDs&& i) {
  core::InlinedVector<FdHandle, 2> hs;
  auto oops = MakeCleanup([&]() {
    for (auto h : hs) {
      auto* perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
      Expunge(perfd);
      CHECK_OK(
          core::syscalls::EpollCtl(efd_, EPOLL_CTL_DEL, perfd->fd, nullptr));
      CHECK_OK(ht_.Free(Coerce(h)));
    }
  });
  for (auto& fd : i.fds) {
    TRY_ASSIGN(auto h, Add(i.h, std::move(fd)));
    hs.push_back(h);
  }
  oops.Cancel();
  return Result::Ok();
}

Result Silo::Affinitize(IoHandler* h) const {
  const auto a = h->GetAffinity();
  if (a == IoHandler::kNoAffinity) {
    h->SetAffinity(id_);
  } else if (a != id_) {
    return core::PreconditionError("Affinity mismatch during add.");
  }
  return Result::Ok();
}

ResultOr<FdHandle> Silo::Add(std::shared_ptr<IoHandler> handler,
                             core::FileDescriptor fd) {
  TRY(Affinitize(handler.get()));

  // Allocate a handle and initialize the PerFd.
  TRY_ASSIGN(auto h, ht_.Allocate());
  auto cleanup = MakeCleanup([&]() { CHECK_OK(ht_.Free(h)); });
  const auto real_h = Coerce(h);
  TRY_ASSIGN(auto* perfd, ht_.Lookup(h));
  perfd->handler = std::move(handler);
  perfd->handler->handles_.push_back(real_h);
  perfd->fd = std::move(fd);
  perfd->handle = real_h;
  perfd->squelch_reads = false;
  perfd->squelch_writes = false;
  perfd->last_activation = Context::Current()->loop_start();

  // Add to epoll set.
  struct epoll_event e;
  e.events = EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLIN | EPOLLOUT | EPOLLET;
  e.data.u64 = static_cast<uint64_t>(h.value());
  TRY(core::syscalls::EpollCtl(efd_, EPOLL_CTL_ADD, perfd->fd, &e));
  cleanup.Cancel();
  idlers_.PushBack(perfd);
  return real_h;
}

Result Silo::Remove(PerFd* perfd) {
  DCHECK(!active_.IsLinked(perfd));
  DCHECK(!idlers_.IsLinked(perfd));
  DCHECK(!runners_.IsLinked(perfd));

  // Remove from epoll set.
  TRY(core::syscalls::EpollCtl(efd_, EPOLL_CTL_DEL, perfd->fd, nullptr));
  auto fd = std::move(perfd->fd);  // closes the fd
  perfd->handler.reset();          // unref the handler

  // Deallocate.
  return ht_.Free(Coerce(perfd->handle));
}

void Silo::RequestRead(FdHandle h) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  perfd->wants_read = true;
  perfd->squelch_reads = false;
  core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
  active_.PushBack(perfd);
}

void Silo::RequestWrite(FdHandle h) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  perfd->wants_write = true;
  perfd->squelch_writes = false;
  core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
  active_.PushBack(perfd);
}

void Silo::Run(FdHandle h, std::move_only_function<void()> fn) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  perfd->fns.emplace_back(std::move(fn));
  if (runners_.IsLinked(perfd)) {
    return;
  }
  runners_.PushBack(perfd);
}

void Silo::Eject(FdHandle h) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  if (ejects_.IsLinked(perfd)) {
    return;
  }
  Expunge(perfd);
  ejects_.PushBack(perfd);
}

bool Silo::ActivationsEmpty() const {
  return active_.Empty() && runners_.Empty();
}

void Silo::RunEjecters() {
  while (!ejects_.Empty()) {
    PerFd* const perfd = &*ejects_.begin();
    core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
    current_ = perfd->handle;

    CHECK_OK(core::syscalls::EpollCtl(efd_, EPOLL_CTL_DEL, perfd->fd, nullptr));
    perfd->handler->HandleEject(std::move(perfd->fd));
    CHECK_OK(ht_.Free(Coerce(current_)));
  }
  current_ = FdHandle::kInvalid;
}

void Silo::RunActives() {
  const auto loop_start = Context::Current()->loop_start();
  while (!active_.Empty()) {
    PerFd* const perfd = &*active_.begin();
    core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);

    current_ = perfd->handle;
    perfd->last_activation = loop_start;

    IoHandler::Outcome read_outcome = IoHandler::Outcome::kClose;
    if (perfd->wants_read) {
      perfd->wants_read = false;
      auto r = perfd->handler->HandleRead(current_, perfd->fd);
      if (r.IsOk()) read_outcome = r.ValueOrDie();
      switch (read_outcome) {
        case IoHandler::Outcome::kFdEagain: {
          // Nominal case. Epoll can trigger again.
          break;
        }
        case IoHandler::Outcome::kYield: {
          // Re-enqueue for more reading.
          perfd->wants_read = true;
          break;
        }
        case IoHandler::Outcome::kSuspend: {
          // Handler will call RequestRead at some point in the future.
          perfd->squelch_reads = true;
          break;
        }
        case IoHandler::Outcome::kClose: {
          // Close is requested, move to the closers list.
          Expunge(perfd);
          closers_.PushBack(perfd);
          break;
        }
      }
    }

    IoHandler::Outcome write_outcome = IoHandler::Outcome::kClose;
    if (perfd->wants_write) {
      perfd->wants_write = false;
      auto r = perfd->handler->HandleWrite(current_, perfd->fd);
      if (r.IsOk()) write_outcome = r.ValueOrDie();
      switch (write_outcome) {
        case IoHandler::Outcome::kFdEagain: {
          // Nominal case. Epoll can trigger again.
          break;
        }
        case IoHandler::Outcome::kYield: {
          // Re-enqueue for more writing.
          perfd->wants_write = true;
          break;
        }
        case IoHandler::Outcome::kSuspend: {
          // Handler will call RequestWrite at some point in the future.
          perfd->squelch_writes = true;
          break;
        }
        case IoHandler::Outcome::kClose: {
          // Close is requested, move to the closers list.
          Expunge(perfd);
          closers_.PushBack(perfd);
          break;
        }
      }
    }

    // RequestRead and RequestWrite can sneak in, so we may already be in a
    // list.
    if (closers_.IsLinked(perfd)) continue;
    if (active_.IsLinked(perfd)) continue;

    // Decide list based on remaining flags.
    if (perfd->wants_read || perfd->wants_write) {
      active_.PushBack(perfd);
    } else {
      idlers_.PushBack(perfd);
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::RunRunners() {
  while (!runners_.Empty()) {
    PerFd* const perfd = &*runners_.begin();
    core::IntrusiveList<PerFd, Shared>::Erase(perfd);
    current_ = perfd->handle;
    while (!perfd->fns.empty()) {
      auto fn = std::move(perfd->fns.front());
      perfd->fns.pop_front();
      fn();
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::RunIdlers() {
  constexpr int kIdlersPerLoop = 1;
  const auto loop_start = Context::Current()->loop_start();
  for (int i = 0; i < kIdlersPerLoop; ++i) {
    if (!idlers_.Empty()) {
      PerFd* const perfd = &*idlers_.begin();
      core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
      current_ = perfd->handle;
      const Duration idle_time = loop_start - perfd->last_activation;
      Result r = Result::Ok();
      if (idle_time > perfd->handler->idle_threshold()) {
        r = perfd->handler->HandleIdle(current_, perfd->fd);
      }
      if (r.IsOk()) {
        // Either not expired or handler accepted the expiration.
        idlers_.PushBack(perfd);
      } else {
        // Handler requsted a close.
        Expunge(perfd);
        closers_.PushBack(perfd);
      }
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::Expunge(PerFd* perfd) {
  perfd->wants_read = false;
  perfd->wants_write = false;
  core::IntrusiveList<PerFd, ActiveIdle>::Erase(perfd);
  core::IntrusiveList<PerFd, Shared>::Erase(perfd);
}

Result Silo::RunClosers() {
  while (!closers_.Empty()) {
    PerFd* const perfd = &*closers_.begin();
    core::IntrusiveList<PerFd, Shared>::Erase(perfd);
    TRY(Remove(perfd));
  }
  return Result::Ok();
}

void Silo::RunShedders() {
  while (!shedders_.Empty()) {
    PerFd* const perfd = &*shedders_.begin();
    core::IntrusiveList<PerFd, Shared>::Erase(perfd);

    // From this FDs handler find all the FDs it owns and shed them all.
    internal::HFDs hfds;
    hfds.h = perfd->handler;  // copy to ensure we keep a ref
    std::list<FdHandle> handles;
    std::swap(handles, perfd->handler->handles_);
    for (const auto h : handles) {
      auto* const pfd = ht_.Lookup(Coerce(h)).ValueOrDie();
      DCHECK_EQ(perfd->handler.get(), pfd->handler.get());
      Expunge(pfd);
      CHECK_OK(core::syscalls::EpollCtl(efd_, EPOLL_CTL_DEL, pfd->fd, nullptr));
      hfds.fds.emplace_back(std::move(pfd->fd));
      CHECK_OK(ht_.Free(Coerce(h)));
    }
    shed_(std::move(hfds));
  }
}

}  // namespace io

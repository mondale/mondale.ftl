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

 private:
  Silo* const s_;
};

}  // namespace

Silo::Silo(int id, Notification* exiting, Mutex* inbound_mu, InList* inbound,
           std::atomic<int>* util, ShedFn sf, PeekFn pf)
    : id_(id),
      inbound_mu_(inbound_mu),
      inbound_(inbound),
      util_(util),
      exiting_(exiting),
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

    // Convert epoll events to list activations.
    for (int i = 0; i < ready_count; ++i) {
      const auto e = events[i].events;
      const auto h = FdHandle(static_cast<int64_t>(events[i].data.u64));

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

      // Add to read list if not already there?
      if (e & EPOLLIN) {
        DCHECK(!readers_.IsLinked(perfd));
        readers_.PushBack(perfd);
      }

      // Add to write list if not already there?
      if (e & EPOLLOUT) {
        DCHECK(!writers_.IsLinked(perfd));
        writers_.PushBack(perfd);
      }
    }

    // Until they are all empty...
    while (!ActivationsEmpty()) {
      RunReaders(&context);
      RunWriters(&context);
      RunRunners();
    }

    // Close doomed file handles.
    while (!closers_.Empty()) {
      PerFd* const perfd = &*closers_.begin();
      core::IntrusiveList<PerFd, Runners>::Erase(perfd);
      TRY(Remove(perfd));
    }

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
    // Check for an affinity race conditio -- route non-matching affinity
    // elsewhere.
    if (i.h->GetAffinity() != id_) {
      shed_(std::move(i));
      continue;
    }
    TRY(Add(std::move(i)));
  }
  return Result::Ok();
}

Result Silo::Add(internal::HFDPair&& i) {
  // Allocate a handle and initialize the PerFd.
  TRY_ASSIGN(auto h, ht_.Allocate());
  TRY_ASSIGN(auto* perfd, ht_.Lookup(h));
  perfd->handler = std::move(i.h);
  perfd->handler->handles_.push_back(Coerce(h));
  perfd->fd = std::move(i.fd);
  perfd->handle = Coerce(h);

  // Add to epoll set.
  struct epoll_event e;
  e.events = EPOLLHUP | EPOLLERR | EPOLLRDHUP | EPOLLIN | EPOLLOUT | EPOLLET;
  e.data.u64 = static_cast<uint64_t>(h.value());
  return core::syscalls::EpollCtl(efd_, EPOLL_CTL_ADD, perfd->fd, &e);
}

Result Silo::Remove(PerFd* perfd) {
  DCHECK(!readers_.IsLinked(perfd));
  DCHECK(!writers_.IsLinked(perfd));
  DCHECK(!runners_.IsLinked(perfd));

  // Remove from epoll set.
  TRY(core::syscalls::EpollCtl(efd_, EPOLL_CTL_DEL, perfd->fd, nullptr));
  static_cast<void>(std::move(perfd->fd));       // closes the fd
  static_cast<void>(std::move(perfd->handler));  // unref the handler

  // Deallocate.
  return ht_.Free(Coerce(perfd->handle));
}

void Silo::RequestRead(FdHandle h) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  if (readers_.IsLinked(perfd)) {
    return;
  }
  readers_.PushBack(perfd);
}

void Silo::RequestWrite(FdHandle h) {
  DCHECK_NE(FdHandle::kInvalid, current_);
  auto* const perfd = ht_.Lookup(Coerce(h)).ValueOrDie();
  if (writers_.IsLinked(perfd)) {
    return;
  }
  writers_.PushBack(perfd);
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

bool Silo::ActivationsEmpty() const {
  return readers_.Empty() && writers_.Empty() && runners_.Empty();
}

void Silo::RunReaders(Context* c) {
  while (!readers_.Empty()) {
    PerFd* const perfd = &*readers_.begin();
    core::IntrusiveList<PerFd, Readers>::Erase(perfd);
    current_ = perfd->handle;
    const auto outcome = perfd->handler->HandleRead(c, current_, perfd->fd);
    switch (outcome) {
      case IoHandler::Outcome::kFdEagain: {
        // Nominal case, nothing to do here. Epoll can trigger again.
        break;
      }
      case IoHandler::Outcome::kYield: {
        // Re-enqueue for more reading.
        readers_.PushBack(perfd);
        break;
      }
      case IoHandler::Outcome::kSuspend: {
        // Nothing to do here, handler will call RequestRead at some point in
        // the future.
        break;
      }
      case IoHandler::Outcome::kClose: {
        // Close is requested, move tot he closers list.
        Expunge(perfd);
        closers_.PushBack(perfd);
        break;
      }
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::RunWriters(Context* c) {
  while (!writers_.Empty()) {
    PerFd* const perfd = &*writers_.begin();
    core::IntrusiveList<PerFd, Writers>::Erase(perfd);
    current_ = perfd->handle;
    const auto outcome = perfd->handler->HandleWrite(c, current_, perfd->fd);
    switch (outcome) {
      case IoHandler::Outcome::kFdEagain: {
        // Nominal case, nothing to do here. Epoll can trigger again.
        break;
      }
      case IoHandler::Outcome::kYield: {
        // Re-enqueue for more reading.
        writers_.PushBack(perfd);
        break;
      }
      case IoHandler::Outcome::kSuspend: {
        // Nothing to do here, handler will call RequestWrite at some point in
        // the future.
        break;
      }
      case IoHandler::Outcome::kClose: {
        // Close is requested, move tot he closers list.
        Expunge(perfd);
        closers_.PushBack(perfd);
        break;
      }
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::RunRunners() {
  while (!runners_.Empty()) {
    PerFd* const perfd = &*runners_.begin();
    core::IntrusiveList<PerFd, Runners>::Erase(perfd);
    current_ = perfd->handle;
    while (!perfd->fns.empty()) {
      auto fn = std::move(perfd->fns.front());
      perfd->fns.pop_front();
      fn();
    }
  }
  current_ = FdHandle::kInvalid;
}

void Silo::Expunge(PerFd* perfd) {
  core::IntrusiveList<PerFd, Readers>::Erase(perfd);
  core::IntrusiveList<PerFd, Writers>::Erase(perfd);
  core::IntrusiveList<PerFd, Runners>::Erase(perfd);
}

}  // namespace io

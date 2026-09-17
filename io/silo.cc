#include <array>

#include "core/handle_table.h"
#include "core/syscalls.h"
#include "io/silo.h"
#include "io/utilization_estimator.h"

namespace io {

Silo::Silo(int id, Notification* exiting, Mutex* inbound_mu, InList* inbound,
           std::atomic<int>* util, ShedFn sf)
    : id_(id),
      inbound_mu_(inbound_mu),
      inbound_(inbound),
      util_(util),
      exiting_(exiting),
      shed_(std::move(sf)) {}

void Silo::ThreadMain() {
  auto ret = ThreadMain2();
  if (!ret.IsOk()) {
    Log(ERROR) << "Thread silo exiting with error: " << ret.ToString();
  }
}

Result Silo::ThreadMain2() {
  core::IntrusiveList<PerFd, Runners> closers;
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
    for (int i = 0; i < ready_count; ++i) {
      const auto e = events[i].events;
      const auto h = FdHandle(static_cast<int64_t>(events[i].data.u64));

      // Lookup handle and get the per-FD object.
      TRY_ASSIGN(auto* perfd, ht_.Lookup(Coerce(h)));

      // Move to error list?
      if ((e & EPOLLERR) || (e & EPOLLHUP) || (e & EPOLLRDHUP)) {
        // Expunge the per-FD object from all lists.
        core::IntrusiveList<PerFd, Readers>::Erase(perfd);
        core::IntrusiveList<PerFd, Writers>::Erase(perfd);
        core::IntrusiveList<PerFd, Runners>::Erase(perfd);

        // Place the per-FD object in the closure list.
        closers.PushBack(perfd);

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
    // Run the readables.
    // Run the writables.
    // Run the closures.

    // Close doomed file handles.
    while (!closers.Empty()) {
      PerFd* const perfd = &*closers.begin();
      core::IntrusiveList<PerFd, Runners>::Erase(perfd);
      TRY(Remove(perfd));
    }

    // Run the inbound admission.
    TRY(RunAdmission());

    // Run load shedding.
    const auto loop_end = MonotonicTime::Now();

    // Report utilization.
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

}  // namespace io

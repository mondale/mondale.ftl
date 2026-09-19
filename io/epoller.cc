#include "core/stateless_random.h"
#include "core/syscalls.h"
#include "io/epoller.h"
#include "io/silo.h"

namespace io {

Epoller::Epoller() {}

Epoller::~Epoller() {
  exiting_.Notify();
  for (const auto& t : threads_) {
    t->thread->Join();
  }
}

// static
ResultOr<std::unique_ptr<Epoller>> Epoller::Build(int silos) {
  if (silos < 1) {
    return core::InvalidArgumentError(strings::Format(
        "Epoller may not be built with < 1 silos [{}].", silos));
  }

  auto ret = std::make_unique<Epoller>();

  std::vector<std::unique_ptr<PerThread>> threads;
  threads.resize(silos);
  for (int i = 0; i < silos; ++i) {
    auto& s = *threads[i];
    TRY_ASSIGN(s.event_fd,
               core::syscalls::EventFd(0, EFD_CLOEXEC | EFD_NONBLOCK));

    // Fire up the silo's thread.
    const std::string name = strings::Format("silo{}", i);
    s.thread = CreateThread(name, [id = i, ev = &s.event_fd, e = &ret->exiting_,
                                   util = &s.utilization, mu = &s.mu,
                                   l = &s.inbound, ep = ret.get()]() {
      base::BecomeForegroundThread();
      auto x = std::make_unique<Silo>(
          id, ev, e, mu, l, util,
          [ep](internal::HFDs&& i) { ep->Route(std::move(i)); },
          [ep](std::vector<int>* utils) { ep->Peek(utils); });
      x->ThreadMain();
    });
  }

  ret->threads_ = std::move(threads);
  return ret;
}

void Epoller::Peek(std::vector<int>* utils) {
  const auto n = threads_.size();
  utils->resize(n);
  for (int i = 0; i < n; ++i) {
    const auto u = std::clamp<int>(
        threads_[i]->utilization.load(std::memory_order_acquire), 1, 99);
    (*utils)[i] = u;
  }
}

int Epoller::SelectSilo() {
  // Weighted random based on utilization, fall back to uniform random.
  const auto n = threads_.size();
  std::vector<int> utils;
  Peek(&utils);

  std::vector<uint32_t> weights;
  weights.resize(n);
  for (int i = 0; i < n; ++i) {
    weights[i] = 100 - utils[i];
  }
  auto maybe_selection = core::WeightedSelect(weights);
  if (maybe_selection.ok()) {
    return maybe_selection.ValueOrDie();
  }
  return core::RandomUniform(0, n);
}

void Epoller::Route(internal::HFDs&& i) {
  int silo = i.h->GetAffinity();
  if (IoHandler::kNoAffinity == silo) {
    silo = SelectSilo();
    i.h->SetAffinity(silo);
  }
  if (silo >= threads_.size()) {
    Log(WARNING) << "IoHandler has broken affinity for silo [" << silo
                 << "]. FD will be closed.";
    return;
  }
  RouteTo(silo, std::move(i));
}

void Epoller::RouteTo(int silo, internal::HFDs&& i) {
  DCHECK_GE(silo, 0);
  DCHECK_LT(silo, threads_.size());
  PerThread& s = *threads_[silo];
  {
    MutexLock l(&s.mu);
    s.inbound.emplace_back(std::move(i));
  }
  // Poke target thread.
  CHECK_OK(core::syscalls::EventFdWrite(s.event_fd, 1));
}

Result Epoller::Register(std::shared_ptr<IoHandler> h,
                         core::FileDescriptor&& fd) {
  core::InlinedVector<core::FileDescriptor, 2> fds;
  fds.emplace_back(std::move(fd));
  Route({std::move(h), std::move(fds)});
  return Result::Ok();
}

}  // namespace io

#include "core/stateless_random.h"
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

  std::vector<std::unique_ptr<PerThread>> threads;
  threads.resize(silos);
  for (int i = 0; i < silos; ++i) {
    //
  }

  auto ret = std::make_unique<Epoller>();
  ret->threads_ = std::move(threads);
  return ret;
}

int Epoller::SelectSilo() {
  // Weighted random based on utilization, fall back to uniform random.
  const auto n = threads_.size();
  std::vector<uint32_t> weights;
  weights.resize(n);
  for (int i = 0; i < n; ++i) {
    // u has range [1, 99]
    const auto u = std::clamp<uint32_t>(
        threads_[i]->utilization.load(std::memory_order_acquire), 1u, 99u);
    weights[i] = 100 - u;
  }
  auto maybe_selection = core::WeightedSelect(weights);
  if (maybe_selection.ok()) {
    return maybe_selection.ValueOrDie();
  }
  return core::RandomUniform(0, n);
}

void Epoller::Route(internal::HFDPair&& i) {
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

void Epoller::RouteTo(int silo, internal::HFDPair&& i) {
  DCHECK_GE(silo, 0);
  DCHECK_LT(silo, threads_.size());
  {
    MutexLock l(&threads_[silo]->mu);
    threads_[silo]->inbound.emplace_back(std::move(i));
  }
  // TODO - poke target thread.
}

Result Epoller::Register(std::shared_ptr<IoHandler> h,
                         core::FileDescriptor&& fd) {
  Route({std::move(h), std::move(fd)});
  return Result::Ok();
}

void Epoller::RequestRead(PollingContext* c, FdHandle h) {}

void Epoller::RequestWrite(PollingContext* c, FdHandle h) {}

void Epoller::Run(PollingContext* c, std::move_only_function<void()> fn) {}

}  // namespace io

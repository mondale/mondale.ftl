#include "io/epoller.h"

namespace io {

Epoller::Epoller() {}

Epoller::~Epoller() {
  exiting_.Notify();
  for (const auto& t : threads_) {
    t.thread->Join();
  }
}

// static
ResultOr<std::unique_ptr<Epoller>> Epoller::Build(int silos) {
  std::vector<PerThread> threads;

  auto ret = std::make_unique<Epoller>();
  ret->threads_ = std::move(threads);
  return ret;
}

int Epoller::SelectSilo() {
  // TODO - need weighted random selection based on utilization
  return 0;
}

void Epoller::Route(Inbound&& i) {
  int silo = i.h->GetAffinity();
  if (Handler::kNoAffinity == silo) {
    silo = SelectSilo();
    i.h->SetAffinity(silo);
  }
  if (silo >= threads_.size()) {
    Log(WARNING) << "Handler has broken affinity for silo [" << silo
                 << "]. FD will be closed.";
    return;
  }
  RouteTo(silo, std::move(i));
}

void Epoller::RouteTo(int silo, Inbound&& i) {
  DCHECK_GE(silo, 0);
  DCHECK_LT(silo, threads_.size());
  {
    MutexLock l(&threads_[silo].mu);
    threads_[silo].inbound.emplace_back(std::move(i));
  }
  // TODO - poke target thread.
}

Result Epoller::Register(std::shared_ptr<Handler> h,
                         core::FileDescriptor&& fd) {
  Route({std::move(h), std::move(fd)});
  return Result::Ok();
}

void Epoller::RequestRead(PollingContext* c, FdHandle h) {}

void Epoller::RequestWrite(PollingContext* c, FdHandle h) {}

void Epoller::Run(PollingContext* c, std::move_only_function<void()> fn) {}

}  // namespace io

#include "io/silo.h"
#include "io/utilization_estimator.h"

namespace io {

Silo::Silo(Notification* exiting, Mutex* inbound_mu, InList* inbound,
           std::atomic<int>* util)
    : inbound_mu_(inbound_mu),
      inbound_(inbound),
      util_(util),
      exiting_(exiting) {}

void Silo::ThreadMain() {
  UtilizationEstimator ue;
  while (!exiting_->HasBeenNotified()) {
    const auto idle_start = MonotonicTime::Now();
    SleepFor(Milliseconds(10));
    // Invoke Epoll.
    const auto busy_start = MonotonicTime::Now();
    // Convert readable edges to the read list.
    // Convert writable edges to the write list.
    // Until they are all empty...
    // Run the readables.
    // Run the writables.
    // Run the closures.
    // Close file handles.
    // Run the inbound.
    const auto loop_end = MonotonicTime::Now();

    // Report utilization.
    const auto busy_duration = loop_end - busy_start;
    const auto idle_duration = busy_start - idle_start;
    ue.Sample(busy_duration.ToNanoseconds(), idle_duration.ToNanoseconds());
    util_->store(ue.Estimate(), std::memory_order_release);

    // Future ... run load shedding.
  }
}

void Silo::GetInList(InList* swapee) {
  MutexLock l(inbound_mu_);
  swapee->swap(*inbound_);
}

}  // namespace io

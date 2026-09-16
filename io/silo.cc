#include "io/silo.h"

namespace io {

Silo::Silo(Notification* exiting, Mutex* inbound_mu, InList* inbound,
           std::atomic<int>* util)
    : inbound_mu_(inbound_mu),
      inbound_(inbound),
      util_(util),
      exiting_(exiting) {}

void Silo::ThreadMain() {
  while (!exiting_->HasBeenNotified()) {
    util_->store(3, std::memory_order_release);
    SleepFor(Milliseconds(10));
  }
}

void Silo::GetInList(InList* swapee) {
  MutexLock l(inbound_mu_);
  swapee->swap(*inbound_);
}

}  // namespace io

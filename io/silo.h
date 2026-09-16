#ifndef IO_SILO_H_
#define IO_SILO_H_

#include <atomic>

#include "core/vocabulary.h"
#include "io/io_handler.h"

namespace io {

class Silo final {
 public:
  using InList = std::list<internal::HFDPair>;
  Silo(Notification* exiting, Mutex* inbound_mu, InList* inbound,
       std::atomic<int>* util);

  void ThreadMain();

 private:
  void GetInList(InList* swapee) LOCKS_EXCLUDED(inbound_mu_);

  Mutex* const inbound_mu_;
  InList* const inbound_ GUARDED_BY(inbound_mu_);
  std::atomic<int>* const util_;
  Notification* const exiting_;
};

}  // namespace io

#endif  // #ifndef IO_SILO_H_

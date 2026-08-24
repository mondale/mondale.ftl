#ifndef CORE_THREAD_H_
#define CORE_THREAD_H_

#include <memory>

#include "core/result.h"

namespace core {
namespace internal {
class ThreadImpl;
}  // namespace internal

class Thread final {
 public:
  // Thread's dtor blocks until the underlying thread has completed execution.
  ~Thread();

  // Blocks caller until this thread has finished executing its target function.
  //
  // May be called repeatedly. Idempotent.
  void Join();

  // Returns true when Join() is done. May be polled as in indicator of
  // deletion readiness.
  bool Joinable();

 private:
  std::unique_ptr<ThreadImpl> impl_;
};

ResultOr<std::unique_ptr<Thread>> CreateThread(absl::string_view name_prefix,
                                               std::function<void()> fn);

Result CreateDetachedThread(absl::string_view name_prefix,
                            std::function<void()> fn);

}  // namespace core

#endif  // #ifndef CORE_THREAD_H_

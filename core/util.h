#ifndef CORE_UTIL_H_
#define CORE_UTIL_H_

#include <iostream>
#include <type_traits>
#include <utility>

#include "base/source_location.h"
#include "core/result.h"

// Main APIs:
//  auto cleanup = util::MakeCleanup([captures]() { DoCleanup(); });
//
//  util::DieElegantlyIfNotOk(result);
//
// Horrible templates follow.

namespace core::util {
namespace internal {

template <typename F>
class CleanupGuard {
 public:
  explicit CleanupGuard(F&& func)
      : func_(std::forward<F>(func)), active_(true) {}

  ~CleanupGuard() noexcept {
    if (active_) {
      func_();
    }
  }

  // Move construction allows returning from MakeCleanup
  CleanupGuard(CleanupGuard&& other) noexcept(
      std::is_nothrow_move_constructible<F>::value)
      : func_(std::move(other.func_)), active_(other.active_) {
    other.active_ = false;
  }

  // Non-copyable and non-assignable
  CleanupGuard(const CleanupGuard&) = delete;
  CleanupGuard& operator=(const CleanupGuard&) = delete;
  CleanupGuard& operator=(CleanupGuard&&) = delete;

  // Optional helper to deactivate cleanup without executing
  void Cancel() noexcept { active_ = false; }

 private:
  F func_;
  bool active_{true};
};
}  // namespace internal

template <typename F>
[[nodiscard]] auto MakeCleanup(F&& func) {
  return internal::CleanupGuard<std::decay_t<F>>(std::forward<F>(func));
}

// Helper to terminate the process elegantly while complaining about 'r' if
// necessary.
void DieElegantlyIfNotOk(
    Result r, base::SourceLocation loc = base::SourceLocation::Current());

}  // namespace core::util

#endif  // #ifndef CORE_UTIL_H_

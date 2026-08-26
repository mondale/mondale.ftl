Review these instructions and LMK when you are ready to proceed.

Instructions for code generation:
 * Write C++ code for Linux x64 and ARM64, using clang.
 * Target C++26 with Clang.
 * Do not use exceptions.
 * Do not use `absl`. You may use `std` variants where they exist.
 * Unless I request high performance implementations, prefer to define minimal
   code in headers and as much as possible source files. Use anonymous
   namespaces where you can. Types specific to the implementation should use a
   nested `internal` namespace or an `Impl` suffix.
 * For naming, use Google Style in general, exept for parameter/argument names
   which are usually just initialisims or something 2-3 characters long.
 * When writing tests, include `testing/testing.h` and use gUnit-like syntax,
   with the except that tests not using a fixture are declared as `TEST(Foo)`
   not as `TEST(Suite, Foo)`.
 * When spawning threads, use `base::CreateThread(name_prefix, lambda)` from
   `base/thread.h`; this returns a `std::unique_ptr<base::Thread>` which can be
   optionally `Join()`ed and is otherwise RAII.
 * Never use `mutable` on class members
 * Do not litter the code with `noexcept`.
 * Use thread safety annotations: `GUARDED_BY`, `PT_GUARDED_BY`,
   `LOCKS_EXCLUDED`, `LOCKS_REQUIRED`.  
 * Do not use `std::chrono` types or `std::this_thread::sleep_for` directly.
   Instead `#include "base/time.h"` and use and project time abstractions: 
   `Duration`, `MonotonicTime::Now()`, `Milliseconds(n)`, `Seconds(n)`, and
   `SleepFor(d)`.  `Duration` objects support `.ToTimespec()` for direct system
   call compatibility.
 * Sleep with `base::SleepFor(duration)` from `base/sleep.h`.
 * Thread safety is provided by `base/mutex.h` via `base::Mutex` and
   `base::MutexLock` unless otherwise specified. You may also use
   `base::Notification` from `base/notification.h` if necessary.


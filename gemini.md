Review these instructions and LMK when you are ready to proceed.

Instructions for code generation:
 * Write C++ code for Linux x64 and ARM64, using clang.
 * The essential vocabulary of the project can be brought in with `#include
   "core/vocabulary.h"`.
 * Use `Result` and `ResultOr<T>` akin to `Status` and `StatusOr<T>`.
 * Create an erroneous `Result` with `Result(Code::kError, "Message")` which
   implicitly converts to `ResultOr<T>`.
 * Create an OK `Result` with `Result::Ok()`.
 * Use `TRY(Expr())` and `TRY_ASSIGN(auto x, Expr())` as your propagation
   macros, akin to `RETURN_IF_ERROR()` and `ASSIGN_OR_RETURN` respectively.
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
 * When spawning threads, use `CreateThread(name_prefix, lambda)` from
   `core/vocabulary.h`; this returns a `std::unique_ptr<Thread>` which can be
   `Join()`ed and is otherwise RAII. Do not use `TRY_ASSIGN` with `CreateThread`
   as the latter does not fail.
 * Never use `mutable` on class members
 * Do not litter the code with `noexcept`.
 * Use thread safety annotations: `GUARDED_BY`, `PT_GUARDED_BY`,
   `LOCKS_EXCLUDED`, `LOCKS_REQUIRED`.  
 * Do not use `std::chrono` types or `std::this_thread::sleep_for` directly.
 * Use `#include "core/vocabulary.h"` and project time abstractions: `Duration`,
   `MonotonicTime::Now()`, `Milliseconds(n)`, `Seconds(n)`, and
   `SleepFor(d)`.  `Duration` objects support `.ToTimespec()` for direct system
   call compatibility.


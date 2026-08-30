Review these instructions and LMK when you are ready to proceed.

## General instructions for code generation:
 * Write C++ code for Linux x64 and ARM64, using clang.
 * Create an erroneous `Result` with `Result(Code::kError, "Message")` which
   implicitly converts to `ResultOr<T>`.
 * Target C++26 with Clang.
 * Unless I request high performance implementations, prefer to define minimal
   code in headers and as much as possible source files. Use anonymous
   namespaces where you can. Types specific to the implementation should use a
   nested `internal` namespace or an `Impl` suffix.
 * For naming, use Google Style in general, except for parameter/argument names
   for methods less than 15 lines long, which are usually just initialisims or
   something 1-3 characters long.
 * Never use `mutable` on class members
 * `unique_ptr`+`make_unique` is encouraged for heap management. `shared_ptr`
   is acceptable to manage shared objects but shared objects should be rare
   unless specifically requested.
 * Do not litter the code with `noexcept`.

## Banned features, libraries, and patterns
 * When no elegant solution is apparent except for use of a banned pattern,
   halt code generation when requested and inquire with the user as to what
   pattern to deploy.
 * Do not use exceptions.
 * Do not use `absl`.
 * Do not use any of the following from `std`: `thread`, `chrono`, coroutines,
   promises, condvars, semaphores, futures, latches, or barriers.

## Vocabulary.
 * The essential vocabulary of the project can be brought in with `#include
   "core/vocabulary.h"` unless otherwise noted.
 * **Core Vocabulary Scoping:** Types and functions brought in via
   `"core/vocabulary.h"` (such as `Thread`, `CreateThread`, `Mutex`, `Duration`,
   `Log`, etc.) reside in the core vocabulary scope/namespace, **never** the
   local module namespace Reference them directly without
   local module prefixes.  
 * Use `Result` and `ResultOr<T>` akin to `Status` and `StatusOr<T>`.
   * Test for OK with `result.IsOk()`.
   * Use `TRY_ASSIGN(Type v, Method(...)` in lieu of `ASSIGN_OR_RETURN`, which
     may return a `Result` or `ResultOr<T>` from the enclosing function.
   * Use `TRY(Method(...))` in lieu of `RETURN_IF_ERROR`, which may return
     a `Result` from the enclosing function.
   * Create an OK `Result` with `Result::Ok()`.
   * Extract a value from a `ResultOr<T>` with `x.ValueOrDie()` not `*x`. Only
     do so when a crash is appropriate when `ResultOr<T>` holds an error.
   * Create a non-OK `Result` with `Result(Code::kError, "Message")`. Do not
     attempt implicit conversions to error by other means.
 * Use `std::unique_ptr<Thread> CreateThread(name_prefix, lambda)` to spawn
   threads if needed.
   * A `Thread` has an *optional* `Join()` which may be called, but the dtor
     also calls `Join()` so only call `Join()` explicitly when needed for
     clarity or correctness.
 * For synchronization, use `Mutex`, `MutexLock`, and `Notification`.
   * `Mutex` also supports `void Await(bl)` and `void AwaitWithTimeout(bl d)`
     where `bl` is a boolean-returning-lambda and `d` is a `Duration`.
   * Use thread safety annotations: `GUARDED_BY`, `PT_GUARDED_BY`,
     `LOCKS_EXCLUDED`, `LOCKS_REQUIRED`.  
   * `Notification` supports `n.Notify()`, `bool n.HasBeenNotified()`,
     `n.WaitForNotification()` and 
     `n.WaitForNotificationWithTimeout(Duration d)`.
 * For sleeping, use `void SleepFor(Duration d)`.
 * `Duration`: 
   * Construct it with helpers: `Nanoseconds(n)`, `Microseconds(u)`,
   `Milliseconds(m)`, `Seconds(s)`. 
   * Supports math against duration and time types. 
   * Raw nanoseconds can be extracted with `d.ToNanoseconds()`.
   * `Duration::Zero()` is the canonical zero duration.
   * Supports `struct timespec ToTimespec()` when a `timespec` is needed.
   * Use `WallTime WallTime::Now()` for timekeeping against `CLOCK_REALTIME`.
   * Use `MonotonicTime MonotonicTime::Now()` for `CLOCK_MONOTONIC`.
 * For invariant checking, use `CHECK(cond)` or a more-specific variant if it
   applies: `CHECK_EQ`, `CHECK_NE`, `CHECK_LT`, `CHECK_LE`, `CHECK_GT`,
   `CHECK_GE`. There is also `CHECK_OK` for types like `Result`.
   * `DCHECK` variants are also supported.
 * For logging, use `Log(INFO) << "Message";` as syntax. Available severities
   are `INFO`, `WARNING`, `ERROR`, `FATAL`, and `DFATAL`.
 * Strings:
   * Use `std::string_view` fluently.
   * In namespace `strings::` use the following:
     * For formatting, use `std::string Format(...)`.
     * For joining, use `std::string Join(container, delimiter)`
     * For numeric parsing, use 
       `ResultOr<T> ParseAs(input, /* optional, default  10 */ base)`.

## Testing.
 * When writing tests, include `testing/testing.h`.
 * Tests do not need a main.
 * Tests should be in an anonymous namespace. Use `using` directives for
   abbreviating commonly-used types. 
 * Tests that have no fixture are declared **with a single argument** as `TEST(Foo) { body; }`
 * Tests with a fixture class are declared as
   `TEST_F(FixtureClass, Foo) { body; }`. `body` may use non-private members
   of the fixture class.
 * Use the `EXPECT` family of macros: `EXPECT_EQ`, `EXPECT_NE`, `EXPECT_LT`,
   `EXPECT_LE`, `EXPECT_GT`, `EXPECT_GE` and `EXPECT_TRUE` for simple
   comparisons.
 * Use `EXPECT_NEAR` for floating point equality.
 * Every `EXPECT` macro has an `ASSERT` variant that returns from the test
   case on failure. Use `ASSERT` when the test case should not proceed if the
   required invariant doesn't hold.
 * Use `EXPECT_THAT(expr, HasSubstr("Foo"))` too look for substrings within
   a string. Use `Not(HasSubstr("Foo"))` to invert the condition.
 * Use `EXPECT_DEATH([...]() -> void { body; }, matcher)` for death tests.
   Matchers for death tests are `DiesWithErrorCode(int)`,
   `StdoutContains(regex)`, and `StderrContains(regex)`.
 * When extracting a value from a `ResultOr<T>`, use `r.ValueOrDie()` and not
   a macro.
 * If mocking is needed:
   * `#include "testing/mock.h"`:
   * Mocks are subclasses of what they mock.
   * This framework uses a comma-separated `MOCK_METHOD[n]` macro, **not**
     standard Google Test parenthetical signatures. Syntax:
     * `MOCK_METHOD<arity>(ret_type, FuncName, arg1, arg2, ..., qualifiers)` 
     * Example: `MOCK_METHOD2(int, Foo, const Bar&, int*, const override)` is a
       mock of `int Foo(const Bar&, int*) const = 0;`. 
     * Match the arity number in the macro name (`MOCK_METHOD2` for 2 arguments,
     * `MOCK_METHOD0` for 0, etc.) and include `override`.  
   * If using a mock, review the following document for examples:
     [FTLMock](https://docs.google.com/document/d/1CTI8WJhhwWGt64TzjlvvvnUHPDLQVerE1sN8Lc_n3Ec/edit?tab=t.0)

Instructions for code generation:
 * The essential vocabulary of the project can be brought in with `#include
   "core/vocabulary.h"`.
 * Use `Result` and `ResultOr<T>` akin to `Status` and `StatusOr<T>`.
 * Create an erroneous `Result` with `Result(Code::kError, "Message")`.
 * Create an OK `Result` with `Result::Ok()`.
 * Use `TRY(Expr())` and `TRY_ASSIGN(auto x, Expr())` as your propogation
   macros, akin to `RETURN_IF_ERROR()` and `ASSIGN_OR_RETURN` respectively.
 * Target C++26 with Clang.
 * Do not use exceptions.
 * Unless I request high performance implementations, prefer to define minimal
   code in headers and as much as possible source files. Use anonymous
   namespaces where you can. Types specific to the implementation should use a
   nested `internal` namespace or an `Impl` suffix.
 * For naming, use Google Style in general, exept for parameter/argument names
   which are usually just initialisims or something 2-3 characters long.

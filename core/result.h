#ifndef CORE_RESULT_H_
#define CORE_RESULT_H_

#include <concepts>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "base/rawlog.h"

namespace core {

enum class BaseCode : uint8_t {
  kOk = 0,

  // Generic error condition.
  kError,

  // Invalid argument.
  kInvalidArgument,

  // Permission denied or authentication failure.
  kPermission,

  // Operation canceled by caller.
  kCanceled,

  // Specified deadline has elapsed.
  kDeadline,

  // Requested object not found.
  kNotFound,

  // System is not in a state to fulfill the request.
  kPrecondition,

  // Not enough of something.
  kExhausted,

  // Temporary, retryable error.
  kUnavailable,

  // Posix error codes.
  kEintr,
  kEnoent,
  kEinval,

  // Unimplemented code path reached.
  kUnimplemented,
};

inline bool IsOk(BaseCode code) { return BaseCode::kOk == code; }
std::string_view ToString(BaseCode bc);
std::ostream& operator<<(std::ostream& out, BaseCode bc);
BaseCode BaseCodeFromErrno(int saved_errno);

struct Code final {
  // Abbreviations for BaseCodes.
  static constexpr BaseCode kOk = BaseCode::kOk;
  static constexpr BaseCode kError = BaseCode::kError;
  static constexpr BaseCode kInvalidArgument = BaseCode::kInvalidArgument;
  static constexpr BaseCode kPermission = BaseCode::kPermission;
  static constexpr BaseCode kCanceled = BaseCode::kCanceled;
  static constexpr BaseCode kDeadline = BaseCode::kDeadline;
  static constexpr BaseCode kNotFound = BaseCode::kNotFound;
  static constexpr BaseCode kPrecondition = BaseCode::kPrecondition;
  static constexpr BaseCode kExhausted = BaseCode::kExhausted;
  static constexpr BaseCode kUnimplemented = BaseCode::kUnimplemented;

  Code() = default;
  static Code Ok() { return Code(); }
  Code(BaseCode c) : code(c) {}

  BaseCode base_code() const { return code; }

  bool operator==(const Code& other) const {
    return base_code() == other.base_code();
  }

  bool operator!=(const Code& other) const {
    return base_code() != other.base_code();
  }

  bool IsOk() const { return ::core::IsOk(base_code()); }
  bool Is(BaseCode bc) const { return base_code() == bc; }

 private:
  friend class Result;
  static constexpr size_t kBytesPerCode = 2;
  static constexpr intptr_t kMask = (1ll << (8 * kBytesPerCode)) - 1;
  static bool ValidBits(uintptr_t x) { return x == (x & kMask); }

  BaseCode code{};
  [[maybe_unused]] int16_t pad16{};
  [[maybe_unused]] int32_t pad32{};
};

// Ensure Code is as trivial as an int64_t return type except for default
// construction which is obliged to zero.
static_assert(sizeof(Code) == sizeof(int64_t), "Code must be exactly 64 bits");
static_assert(alignof(Code) <= alignof(int64_t),
              "Code alignment must fit standard register usage");
static_assert(std::is_trivially_copyable_v<Code>,
              "Code must be trivially copyable to pass in registers");
static_assert(
    std::is_standard_layout_v<Code>,
    "Code must be standard layout for predictable C-style ABI layout");
static_assert(std::is_trivially_destructible_v<Code>,
              "Code destructor must be trivial");

inline bool IsOk(Code c) { return BaseCode::kOk == c.base_code(); }
std::string_view ToString(Code c);
std::ostream& operator<<(std::ostream& out, Code c);

// Code and BaseCode can be freely compared for equality.
inline bool operator==(Code c, BaseCode bc) { return c.base_code() == bc; }
inline bool operator==(BaseCode bc, Code c) { return c == bc; }
inline bool operator!=(Code c, BaseCode bc) { return c.base_code() != bc; }
inline bool operator!=(BaseCode bc, Code c) { return c != bc; }

Code CodeFromErrno(int saved_errno);

class [[nodiscard]] Result final {
 public:
  Result() = default;
  Result(Code c) : rep(c) {
    RAW_DCHECK(!ErpEngaged()) << std::hex << rep_bits();
  }
  Result(BaseCode bc) : Result(Code(bc)) {
    RAW_DCHECK(!ErpEngaged()) << std::hex << rep_bits();
  }
  Result(Code c, std::string_view m);
  Result(BaseCode bc, std::string_view m) : Result(Code(bc), m) {}

  ~Result() {
    if (ErpEngaged()) DropRef();
  }

  static Result Ok() { return Result(); }

  Code code() const { return ErpEngaged() ? rep.erp->code : rep.code; }
  BaseCode base_code() const { return code().base_code(); }
  bool IsOk() const { return ::core::IsOk(base_code()); }
  bool Is(Code c) const { return code() == c; }
  bool Is(BaseCode bc) const { return base_code() == bc; }

  std::string ToString() const;
  std::string_view message() const;

  // Move Constructor
  Result(Result&& other) : rep(other.rep) {
    if (ErpEngaged()) other.Disengage();  // Inherit other's ref.
  }

  // Move Assignment Operator
  Result& operator=(Result&& other) {
    if (this != &other) {
      if (ErpEngaged()) DropRef();  // Clean up current allocation first
      rep = other.rep;
      if (ErpEngaged()) other.Disengage();  // Inherit other's ref.
    }
    return *this;
  }

  // Copy Constructor
  Result(const Result& other) {
    rep.bits = other.rep.bits;
    if (ErpEngaged()) {
      rep.erp->refs++;
    }
  }

  // Copy Assignment Operator
  Result& operator=(const Result& other) {
    if (this != &other) {
      if (ErpEngaged()) DropRef();  // Clean up current allocation first
      rep = other.rep;
      if (ErpEngaged()) {
        rep.erp->refs++;
      }
    }
    return *this;
  }

  // Testing aids.
  // Returns true when the underlying Extended Representation Pointer is
  // engaged, i.e., this object holds a heap allocation.
  bool ErpEngaged() const { return !Code::ValidBits(rep.bits); }

  // Returns the raw bits of rep.
  intptr_t rep_bits() const { return rep.bits; }

  // Returns the refcount or zero if not engaged.
  int refs() const { return ErpEngaged() ? rep.erp->refs : 0; }

 private:
  struct ExtendedRep {
    ExtendedRep(Code c, std::string_view m);
    int refs;
    Code code;
    std::string message;
  };

  union Rep {
    Code code;
    intptr_t bits;
    ExtendedRep* erp;
  } rep{};

  void DropRef();

  // The ref has been trasnferred to another instance. Revert to the unengaged
  // variant.
  void Disengage();
};

static_assert(sizeof(Result) == 8, "Result should be precisely 64 bits");

inline bool IsOk(Result r) { return r.IsOk(); }
std::string ToString(Result r);
std::ostream& operator<<(std::ostream& out, const Result& r);

Result ResultFromErrno(int saved_errno);

template <typename T>
class [[nodiscard]] ResultOr;

namespace internal {

struct ErrorPropagator {
  Result result;

  explicit ErrorPropagator(Result r) : result(std::move(r)) {}
  explicit ErrorPropagator(Code c) : result(c) {}
  explicit ErrorPropagator(BaseCode bc) : result(bc) {}

  operator Result() && { return std::move(result); }
  operator Code() const { return result.code(); }
  operator BaseCode() const { return result.base_code(); }

  template <typename T>
  operator ResultOr<T>() && {
    return std::move(result);
  }
};

template <typename T>
struct is_error_propagator : std::false_type {};

template <>
struct is_error_propagator<ErrorPropagator> : std::true_type {};

template <typename T>
inline constexpr bool is_error_propagator_v = is_error_propagator<T>::value;

inline bool IsStatusOk(BaseCode bc) { return ::core::IsOk(bc); }
inline bool IsStatusOk(Code c) { return ::core::IsOk(c); }
inline bool IsStatusOk(const Result& r) { return r.IsOk(); }
template <typename T>
inline bool IsStatusOk(const ResultOr<T>& ro) {
  return ro.ok();
}

inline Result ExtractResult(BaseCode bc) { return Result(bc); }
inline Result ExtractResult(Code c) { return Result(c); }
inline Result ExtractResult(Result r) { return r; }
template <typename T>
inline Result ExtractResult(ResultOr<T> ro) {
  return std::move(ro).result();
}

}  // namespace internal

template <typename T>
class [[nodiscard]] ResultOr final {
 public:
  using value_type = T;

  // Disallow default construction (requires either a Result or a Value)
  ResultOr() = delete;

  // Construct from a non-OK Result.
  ResultOr(Result r) : storage_(std::move(r)) {
    RAW_CHECK(!result().IsOk())
        << "Cannot construct ResultOr with an OK Result; use a value instead.";
  }

  ResultOr& operator=(Result r) {
    RAW_CHECK(!r.IsOk())
        << "Cannot assign an OK Result to ResultOr; use a value instead.";
    storage_ = std::move(r);
    return *this;
  }

  // Implicit conversion from BaseCode or Code error
  ResultOr(BaseCode bc) : ResultOr(Result(bc)) {}
  ResultOr(Code c) : ResultOr(Result(c)) {}

  // Generic value constructor: explicitly disables ErrorPropagator
  // so it won't attempt to construct T from ErrorPropagator
  template <typename U = T>
    requires(!std::is_same_v<std::decay_t<U>, ResultOr<T>> &&
             !std::is_same_v<std::decay_t<U>, Result> &&
             !internal::is_error_propagator_v<std::decay_t<U>> &&
             std::is_constructible_v<T, U &&>)
  ResultOr(U&& value)
      : storage_(std::in_place_type<T>, std::forward<U>(value)) {}

  // In-place construction helper
  template <typename... Args>
    requires std::is_constructible_v<T, Args...>
  explicit ResultOr(std::in_place_t, Args&&... args)
      : storage_(std::in_place_type<T>, std::forward<Args>(args)...) {}

  ~ResultOr() = default;

  // Copy / Move semantics automatically generated via std::variant
  ResultOr(const ResultOr&) = default;
  ResultOr& operator=(const ResultOr&) = default;
  ResultOr(ResultOr&&) noexcept(std::is_nothrow_move_constructible_v<T>) =
      default;
  ResultOr& operator=(ResultOr&&) noexcept(
      std::is_nothrow_move_assignable_v<T>) = default;

  [[nodiscard]] bool ok() const noexcept {
    return std::holds_alternative<T>(storage_);
  }

  [[nodiscard]] const Result& result() const& noexcept {
    if (ok()) {
      static const Result* perma_ok = new Result();
      return *perma_ok;
    }
    return std::get<Result>(storage_);
  }

  [[nodiscard]] Result result() && {
    if (ok()) {
      return Result::Ok();
    }
    return std::get<Result>(std::move(storage_));
  }

  [[nodiscard]] const T& ValueOrDie() const& {
    CheckOk();
    return std::get<T>(storage_);
  }

  [[nodiscard]] T& ValueOrDie() & {
    CheckOk();
    return std::get<T>(storage_);
  }

  [[nodiscard]] const T&& ValueOrDie() const&& {
    CheckOk();
    return std::get<T>(std::move(storage_));
  }

  [[nodiscard]] T&& ValueOrDie() && {
    CheckOk();
    return std::get<T>(std::move(storage_));
  }

  template <typename U>
  [[nodiscard]] T ValueOr(U&& default_value) const& {
    if (ok()) {
      return std::get<T>(storage_);
    }
    return static_cast<T>(std::forward<U>(default_value));
  }

  template <typename U>
  [[nodiscard]] T value_or(U&& default_value) && {
    if (ok()) {
      return std::get<T>(std::move(storage_));
    }
    return static_cast<T>(std::forward<U>(default_value));
  }

 private:
  void CheckOk() const {
    RAW_CHECK(ok()) << "Attempted to access value of non-OK ResultOr"
                    << result();
  }

  std::variant<Result, T> storage_;
};

}  // namespace core

// Macro concatenation helpers.
#define CORE_IMPL_CONCAT_INNER(x, y) x##y
#define CORE_IMPL_CONCAT(x, y) CORE_IMPL_CONCAT_INNER(x, y)

// Early-return on error for expressions returning BaseCode, Code, Result, or
// ResultOr<T>.
#define TRY(expr)                                                           \
  do {                                                                      \
    auto&& _core_status_val = (expr);                                       \
    if (!::core::internal::IsStatusOk(_core_status_val)) {                  \
      return ::core::internal::ErrorPropagator(                             \
          ::core::internal::ExtractResult(                                  \
              std::forward<decltype(_core_status_val)>(_core_status_val))); \
    }                                                                       \
  } while (0)

// Evaluates `rexpr` (which returns ResultOr<T>), returning early on error or
// assigning the unwrapped T value to `lhs`.
#define TRY_ASSIGN(lhs, rexpr) \
  CORE_TRY_ASSIGN_IMPL(CORE_IMPL_CONCAT(_core_res_or_, __LINE__), lhs, rexpr)

#define CORE_TRY_ASSIGN_IMPL(status_var, lhs, rexpr)             \
  auto status_var = (rexpr);                                     \
  if (!::core::internal::IsStatusOk(status_var)) {               \
    return ::core::internal::ErrorPropagator(                    \
        ::core::internal::ExtractResult(std::move(status_var))); \
  }                                                              \
  lhs = std::move(status_var).ValueOrDie()

#endif  // #ifndef CORE_RESULT_H_

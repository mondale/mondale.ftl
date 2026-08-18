#ifndef CORE_RESULT_H_
#define CORE_RESULT_H_

#include <concepts>
#include <ostream>
#include <string>
#include <string_view>

namespace core {

enum class BaseCode : uint8_t {
  kOk = 0,

  // Generic error condition.
  kError = 1,

  // Invalid argument.
  kInvalidArgument = 2,

  // Permission denied or authentication failure.
  kPermission = 3,

  // Operation canceled by caller.
  kCanceled = 4,

  // Specified deadline has elapsed.
  kDeadline = 5,

  // Requested object not found.
  kNotFound = 6,

  // System is not in a state to fulfill the request.
  kPrecondition = 7,

  // Not enough of something.
  kExhausted = 8,

  // Unimplemented code path reached.
  kUnimplemented = 9,
};

inline bool IsOk(BaseCode code) { return BaseCode::kOk == code; }
std::string_view ToString(BaseCode bc);
std::ostream& operator<<(std::ostream& out, BaseCode bc);

struct Code final {
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
static_assert(sizeof(Code) == 8, "Code should be precisely 64 bits");

inline bool IsOk(Code c) { return BaseCode::kOk == c.base_code(); }
std::string_view ToString(Code c);
std::ostream& operator<<(std::ostream& out, Code c);

// Code and BaseCode can be freely compared for equality.
inline bool operator==(Code c, BaseCode bc) { return c.base_code() == bc; }
inline bool operator==(BaseCode bc, Code c) { return c == bc; }
inline bool operator!=(Code c, BaseCode bc) { return c.base_code() != bc; }
inline bool operator!=(BaseCode bc, Code c) { return c != bc; }

class Result final {
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
  // Returns true when the underlying object holds a heap allocation.
  bool ErpEngaged() const { return !Code::ValidBits(rep.bits); }

  // Returns the raw bits of rep.
  intptr_t rep_bits() const { return rep.bits; }

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

}  // namespace core

#endif  // #ifndef CORE_RESULT_H_

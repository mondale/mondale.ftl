#ifndef CORE_HANDLE_H_
#define CORE_HANDLE_H_

#include <concepts>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <string>

// Main API.
// HandleType(Index, int64_t);
// Also provides Index::kInvalid.
//
// Horrible templates follow.

namespace core {
namespace internal {

// Forward declaration
template <typename Tag, std::integral T>
class Handle;

template <typename Tag, std::integral T>
class Handle final {
 public:
  using value_type = T;
  using tag_type = Tag;

  static constexpr T kInvalidValue = std::numeric_limits<T>::max();

  // Declared inside class where Handle is still being defined
  static const Handle kInvalid;

  constexpr Handle() noexcept : value_(kInvalidValue) {}
  explicit constexpr Handle(T val) noexcept : value_(val) {}

  [[nodiscard]] constexpr bool IsValid() const noexcept {
    return value_ != kInvalidValue;
  }

  [[nodiscard]] constexpr T value() const noexcept { return value_; }

  constexpr bool operator==(const Handle&) const = default;
  constexpr bool operator!=(const Handle&) const = default;

  // String formatting
  [[nodiscard]] std::string ToString() const {
    if (!IsValid()) {
      return "Invalid";
    }
    return std::to_string(value_);
  }

  // Deleted arithmetic
  Handle operator+(auto) = delete;
  Handle operator-(auto) = delete;
  Handle operator*(auto) = delete;
  Handle operator/(auto) = delete;
  Handle operator%(auto) = delete;
  Handle& operator++() = delete;
  Handle& operator--() = delete;

 private:
  T value_;
};

// Defined outside once Handle<Tag, T> is a complete, literal type
template <typename Tag, std::integral T>
inline constexpr Handle<Tag, T> Handle<Tag, T>::kInvalid{
    Handle<Tag, T>::kInvalidValue};

// Stream insertion operator in the same namespace for ADL lookup
template <typename Tag, std::integral T>
std::ostream& operator<<(std::ostream& os, const Handle<Tag, T>& handle) {
  return os << handle.ToString();
}

}  // namespace internal
}  // namespace core

// std::hash specialization for all hardened::Handle instantiations
template <typename Tag, std::integral T>
struct std::hash<core::internal::Handle<Tag, T>> {
  std::size_t operator()(
      const core::internal::Handle<Tag, T>& handle) const noexcept {
    return std::hash<T>{}(handle.value());
  }
};
#define HANDLE_TYPE(Name, Underlying) \
  struct Name##_Tag {};               \
  using Name = ::core::internal::Handle<Name##_Tag, Underlying>

#endif  // #ifndef CORE_HANDLE_H_

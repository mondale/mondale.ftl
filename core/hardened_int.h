#ifndef CORE_HARDENED_INT_H_
#define CORE_HARDENED_INT_H_

#include <concepts>
#include <cstdint>
#include <functional>
#include <ostream>
#include <string>

namespace core::internal {

template <typename Tag, std::integral T>
class HardenedInt {
 public:
  using value_type = T;
  using tag_type = Tag;

  // Constructors
  constexpr HardenedInt() noexcept : value_(0) {}
  explicit constexpr HardenedInt(T val) noexcept : value_(val) {}

  // Explicit value retrieval
  [[nodiscard]] constexpr T value() const noexcept { return value_; }

  // String representation
  [[nodiscard]] std::string ToString() const { return std::to_string(value_); }

  // Strict comparisons (supports total ordering: <, <=, ==, !=, >=, >)
  constexpr auto operator<=>(const HardenedInt&) const = default;

  // Unary operators
  constexpr HardenedInt operator+() const noexcept { return *this; }
  constexpr HardenedInt operator-() const noexcept {
    return HardenedInt(-value_);
  }

  // Compound assignment operators
  constexpr HardenedInt& operator+=(HardenedInt rhs) noexcept {
    value_ += rhs.value_;
    return *this;
  }

  constexpr HardenedInt& operator-=(HardenedInt rhs) noexcept {
    value_ -= rhs.value_;
    return *this;
  }

  constexpr HardenedInt& operator*=(HardenedInt rhs) noexcept {
    value_ *= rhs.value_;
    return *this;
  }

  constexpr HardenedInt& operator/=(HardenedInt rhs) noexcept {
    value_ /= rhs.value_;
    return *this;
  }

  constexpr HardenedInt& operator%=(HardenedInt rhs) noexcept {
    value_ %= rhs.value_;
    return *this;
  }

  // Binary arithmetic operators (strict type matching only)
  constexpr friend HardenedInt operator+(HardenedInt lhs,
                                         HardenedInt rhs) noexcept {
    return lhs += rhs;
  }

  constexpr friend HardenedInt operator-(HardenedInt lhs,
                                         HardenedInt rhs) noexcept {
    return lhs -= rhs;
  }

  constexpr friend HardenedInt operator*(HardenedInt lhs,
                                         HardenedInt rhs) noexcept {
    return lhs *= rhs;
  }

  constexpr friend HardenedInt operator/(HardenedInt lhs,
                                         HardenedInt rhs) noexcept {
    return lhs /= rhs;
  }

  constexpr friend HardenedInt operator%(HardenedInt lhs,
                                         HardenedInt rhs) noexcept {
    return lhs %= rhs;
  }

  // Increment and Decrement
  constexpr HardenedInt& operator++() noexcept {
    ++value_;
    return *this;
  }

  constexpr HardenedInt operator++(int) noexcept {
    HardenedInt temp = *this;
    ++(*this);
    return temp;
  }

  constexpr HardenedInt& operator--() noexcept {
    --value_;
    return *this;
  }

  constexpr HardenedInt operator--(int) noexcept {
    HardenedInt temp = *this;
    --(*this);
    return temp;
  }

 private:
  T value_;
};

// Stream insertion
template <typename Tag, std::integral T>
std::ostream& operator<<(std::ostream& os, const HardenedInt<Tag, T>& val) {
  return os << val.ToString();
}

}  // namespace core::internal

// Hashing support
template <typename Tag, std::integral T>
struct std::hash<core::internal::HardenedInt<Tag, T>> {
  std::size_t operator()(
      const core::internal::HardenedInt<Tag, T>& val) const noexcept {
    return std::hash<T>{}(val.value());
  }
};

#define HARDENED_INT_TYPE(Name, Underlying) \
  struct Name##_Tag {};                     \
  using Name = ::core::internal::HardenedInt<Name##_Tag, Underlying>

#endif  // #ifndef CORE_HARDENED_INT_H_

#ifndef TESTING_COMPARE_H_
#define TESTING_COMPARE_H_

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

namespace testing::internal {

struct Compare {
 private:
  // Helper for floating-point near-equality (ULP / Epsilon check)
  template <typename T>
  static constexpr bool AlmostEqualUlps(T a, T b, int maxUlpsDiff = 4) {
    if (std::isnan(a) || std::isnan(b)) return false;
    if (a == b) return true;  // Handles infinities and exact matches

    // Fallback to absolute difference near zero
    T diff = std::abs(a - b);
    if (diff < std::numeric_limits<T>::epsilon()) return true;

    // Scale tolerance by magnitude for general cases
    return diff <= std::numeric_limits<T>::epsilon() *
                       std::max(std::abs(a), std::abs(b)) * maxUlpsDiff;
  }

 public:
  template <typename T, typename U>
  static constexpr bool Eq(const T& lhs, const U& rhs) {
    using CommonType = std::common_type_t<T, U>;

    // Floating-point path
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
      return AlmostEqualUlps(static_cast<CommonType>(lhs),
                             static_cast<CommonType>(rhs));
    }
    // Integer path
    else if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
      return std::cmp_equal(lhs, rhs);
    }
    // Fallback for user types / pointers / mixed types
    else {
      return lhs == rhs;
    }
  }

  template <typename T, typename U>
  static constexpr bool Ne(const T& lhs, const U& rhs) {
    return !Eq(lhs, rhs);
  }

  template <typename T, typename U>
  static constexpr bool Lt(const T& lhs, const U& rhs) {
    if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
      return std::cmp_less(lhs, rhs);
    } else {
      return lhs < rhs;
    }
  }

  template <typename T, typename U>
  static constexpr bool Le(const T& lhs, const U& rhs) {
    if constexpr (std::is_integral_v<T> && std::is_integral_v<U>) {
      return std::cmp_less_equal(lhs, rhs);
    } else {
      return lhs <= rhs;
    }
  }

  template <typename T, typename U>
  static constexpr bool Gt(const T& lhs, const U& rhs) {
    return !Le(lhs, rhs);
  }

  template <typename T, typename U>
  static constexpr bool Ge(const T& lhs, const U& rhs) {
    return !Lt(lhs, rhs);
  }

  // 1. Explicit Absolute Error Tolerance
  template <typename T, typename U, typename AbsErr>
  static constexpr bool Near(const T& lhs, const U& rhs,
                             const AbsErr& abs_error) {
    // Handle NaNs
    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
      if (std::isnan(lhs) || std::isnan(rhs)) return false;
    }

    // Exact match covers infinities
    if (lhs == rhs) return true;

    using CommonType = std::common_type_t<T, U, AbsErr>;
    auto diff = (lhs > rhs) ? (lhs - rhs) : (rhs - lhs);
    return static_cast<CommonType>(diff) <= static_cast<CommonType>(abs_error);
  }

  // 2. Default ULP / Relative Epsilon Near Check
  template <typename T, typename U>
  static constexpr bool Near(const T& lhs, const U& rhs) {
    using CommonType = std::common_type_t<T, U>;

    if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) {
      CommonType l = static_cast<CommonType>(lhs);
      CommonType r = static_cast<CommonType>(rhs);

      if (std::isnan(l) || std::isnan(r)) return false;
      if (l == r) return true;

      CommonType diff = std::abs(l - r);
      CommonType eps = std::numeric_limits<CommonType>::epsilon();

      // Check if difference falls within 4 ULPs scaled by magnitude
      return diff <= eps * std::max(std::abs(l), std::abs(r)) *
                         static_cast<CommonType>(4);
    } else {
      return Eq(lhs, rhs);
    }
  }
};

}  // namespace testing::internal

#endif  // #ifndef TESTING_COMPARE_H_

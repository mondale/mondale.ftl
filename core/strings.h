#ifndef CORE_STRINGS_H_
#define CORE_STRINGS_H_

#include <concepts>
#include <format>
#include <initializer_list>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace core::strings {

// Main APIs:
// std::string Format(...)
// e.g.:
//   Format("{} and {}", "a", "b");   // a and b
//   Format("{1} {0}", "a", "b");     // b a
//   Format("{:.2f}", 3.14159);       // 3.14
//
// std::string Join(Container, Delimiter);
// e.g.:
//   Join({"a", "bc", "d"}, ", ");    // a, bc, d
//
// Horrible template details follow.

template <typename... Args>
std::string Format(std::format_string<Args...> fmt, Args&&... args) {
  return std::format(fmt, std::forward<Args>(args)...);
}

template <std::ranges::input_range Container>
  requires std::convertible_to<std::ranges::range_value_t<Container>,
                               std::string_view>
std::string Join(const Container& c, std::string_view delimiter) {
  auto begin = std::ranges::begin(c);
  auto end = std::ranges::end(c);

  if (begin == end) {
    return {};
  }

  // Single-pass size calculation for forward ranges to pre-allocate exact
  // memory.
  if constexpr (std::ranges::forward_range<Container>) {
    std::size_t total_size = 0;
    std::size_t count = 0;
    for (auto it = begin; it != end; ++it) {
      total_size += std::string_view(*it).size();
      ++count;
    }
    if (count > 1) {
      total_size += delimiter.size() * (count - 1);
    }

    std::string result;
    result.reserve(total_size);

    auto it = begin;
    result.append(std::string_view(*it));
    for (++it; it != end; ++it) {
      result.append(delimiter);
      result.append(std::string_view(*it));
    }
    return result;
  } else {
    // Fallback for single-pass input ranges.
    std::string result;
    auto it = begin;
    result.append(std::string_view(*it));
    for (++it; it != end; ++it) {
      result.append(delimiter);
      result.append(std::string_view(*it));
    }
    return result;
  }
}

// Overload to support braced-init-lists like Join({"a", "bc", "d"}, ", ")
inline std::string Join(std::initializer_list<std::string_view> il,
                        std::string_view delimiter) {
  return Join<std::initializer_list<std::string_view>>(il, delimiter);
}

}  // namespace core::strings

#endif  // #ifndef CORE_STRINGS_H_

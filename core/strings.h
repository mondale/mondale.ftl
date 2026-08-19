#ifndef CORE_STRINGS_H_
#define CORE_STRINGS_H_

#include <charconv>
#include <concepts>
#include <format>
#include <initializer_list>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "core/result.h"

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
// const auto ParseAs<[integer type]>(String);
//
// auto Split(String, Delimiter);
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

template <typename T>
  requires std::is_arithmetic_v<T>
ResultOr<T> ParseAs(std::string_view input, int base = 10) {
  if (input.empty()) {
    return Code::kInvalidArgument;
  }

  T value{};
  std::from_chars_result res;

  if constexpr (std::is_integral_v<T>) {
    res =
        std::from_chars(input.data(), input.data() + input.size(), value, base);
  } else if constexpr (std::is_floating_point_v<T>) {
    res = std::from_chars(input.data(), input.data() + input.size(), value);
  }

  if (res.ec == std::errc::invalid_argument) {
    return Code::kInvalidArgument;
  }
  if (res.ec == std::errc::result_out_of_range) {
    return Code::kInvalidArgument;
  }
  if (res.ptr != input.data() + input.size()) {
    return Code::kInvalidArgument;
  }

  return value;
}

// Pass-through specialization for string_view
template <typename T>
  requires std::is_same_v<T, std::string_view>
ResultOr<std::string_view> ParseAs(std::string_view input, int = 10) {
  return input;
}

class SplitView : public std::ranges::view_interface<SplitView> {
 public:
  class Iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = std::string_view;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::string_view*;
    using reference = const std::string_view&;

    constexpr Iterator() = default;
    constexpr Iterator(std::string_view input, std::string_view delimiter,
                       bool at_end)
        : remaining_(input), delimiter_(delimiter), at_end_(at_end) {
      if (!at_end_) {
        Advance();
      }
    }

    constexpr std::string_view operator*() const { return current_; }
    constexpr const std::string_view* operator->() const { return &current_; }

    constexpr Iterator& operator++() {
      if (!at_end_) {
        Advance();
      }
      return *this;
    }

    constexpr Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr bool operator==(const Iterator& other) const {
      if (at_end_ && other.at_end_) return true;
      if (at_end_ != other.at_end_) return false;
      return remaining_.data() == other.remaining_.data() &&
             remaining_.size() == other.remaining_.size();
    }

   private:
    constexpr void Advance() {
      if (has_finished_) {
        at_end_ = true;
        return;
      }

      if (delimiter_.empty()) {
        if (remaining_.empty()) {
          at_end_ = true;
          return;
        }
        current_ = remaining_.substr(0, 1);
        remaining_.remove_prefix(1);
        return;
      }

      auto pos = remaining_.find(delimiter_);
      if (pos == std::string_view::npos) {
        current_ = remaining_;
        remaining_ = {};
        has_finished_ = true;
      } else {
        current_ = remaining_.substr(0, pos);
        remaining_.remove_prefix(pos + delimiter_.size());
      }
    }

    std::string_view remaining_{};
    std::string_view delimiter_{};
    std::string_view current_{};
    bool has_finished_ = false;
    bool at_end_ = true;
  };

  constexpr SplitView(std::string_view input, std::string_view delimiter)
      : input_(input), delimiter_(delimiter) {}

  constexpr Iterator begin() const {
    return Iterator(input_, delimiter_, false);
  }
  constexpr Iterator end() const { return Iterator(input_, delimiter_, true); }

 private:
  std::string_view input_;
  std::string_view delimiter_;
};

inline constexpr SplitView Split(std::string_view input,
                                 std::string_view delimiter) {
  return SplitView(input, delimiter);
}

}  // namespace core::strings

#endif  // #ifndef CORE_STRINGS_H_

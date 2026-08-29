#ifndef BASE_FLAGS_H_
#define BASE_FLAGS_H_

#include <charconv>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace base::internal {

struct FlagRegistrationNode {
  const char* cohort;
  const char* name;
  void* storage_ptr;
  bool (*parse_and_validate)(std::string_view val, std::string* err,
                             const void* builder_ptr);
  const void* builder_ptr;
  FlagRegistrationNode* next;
};

void RegisterFlag(FlagRegistrationNode* node);

template <typename T>
class FlagBuilder {
 public:
  FlagBuilder(T* ptr, const char* cohort, const char* name, T default_val)
      : ptr_(ptr), default_val_(std::move(default_val)) {
    node_.cohort = cohort;
    node_.name = name;
    node_.storage_ptr = ptr;
    node_.parse_and_validate = [](std::string_view sv, std::string* err,
                                  const void* bp) {
      return static_cast<const FlagBuilder*>(bp)->ValidateAndSet(sv, err);
    };
    node_.builder_ptr = this;
    node_.next = nullptr;
    ::base::internal::RegisterFlag(&node_);
  }

  FlagBuilder& Ge(T min_val) {
    min_val_ = std::move(min_val);
    is_exclusive_min_ = false;
    return *this;
  }

  FlagBuilder& Gt(T min_val) {
    min_val_ = std::move(min_val);
    is_exclusive_min_ = true;
    return *this;
  }

  FlagBuilder& Le(T max_val) {
    max_val_ = std::move(max_val);
    is_exclusive_max_ = false;
    return *this;
  }

  FlagBuilder& Lt(T max_val) {
    max_val_ = std::move(max_val);
    is_exclusive_max_ = true;
    return *this;
  }

  bool ValidateAndSet(std::string_view sv, std::string* err) const {
    T parsed{};
    if constexpr (std::is_same_v<T, std::string>) {
      parsed = std::string(sv);
    } else if constexpr (std::is_integral_v<T>) {
      auto [p, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), parsed);
      if (ec != std::errc() || p != sv.data() + sv.size()) {
        if (err) {
          *err = std::format("Failed to parse integer from value '{}'", sv);
        }
        return false;
      }
    } else {
      parsed = T(sv);
    }

    if (min_val_) {
      bool violated =
          is_exclusive_min_ ? (parsed <= *min_val_) : (parsed < *min_val_);
      if (violated) {
        if (err) {
          std::string min_str = std::format("{}", *min_val_);
          std::string max_str = max_val_ ? std::format("{}", *max_val_) : "+8";
          char left_char = is_exclusive_min_ ? '(' : '[';
          char right_char = max_val_ ? (is_exclusive_max_ ? ')' : ']') : ')';
          *err = std::format(
              "Value {} is out of bounds: must be within {}{}, {}{}", parsed,
              left_char, min_str, max_str, right_char);
        }
        return false;
      }
    }

    if (max_val_) {
      bool violated =
          is_exclusive_max_ ? (parsed >= *max_val_) : (parsed > *max_val_);
      if (violated) {
        if (err) {
          std::string min_str = min_val_ ? std::format("{}", *min_val_) : "-8";
          std::string max_str = std::format("{}", *max_val_);
          char left_char = min_val_ ? (is_exclusive_min_ ? '(' : '[') : '(';
          char right_char = is_exclusive_max_ ? ')' : ']';
          *err = std::format(
              "Value {} is out of bounds: must be within {}{}, {}{}", parsed,
              left_char, min_str, max_str, right_char);
        }
        return false;
      }
    }

    *ptr_ = std::move(parsed);
    return true;
  }

 private:
  T* ptr_;
  T default_val_;
  bool is_exclusive_min_ = false;
  std::optional<T> min_val_;
  bool is_exclusive_max_ = false;
  std::optional<T> max_val_;
  FlagRegistrationNode node_;
};

}  // namespace base::internal

namespace base {

bool ParseFlags(int argc, char* argv[], std::string* err);

}  // namespace base

#define FLAG_COHORT(cohort_name)                              \
  namespace base_flag_cohort_##cohort_name {                  \
    inline constexpr std::string_view kCohort = #cohort_name; \
  }                                                           \
  using base_flag_cohort_##cohort_name::kCohort

#define FLAG(type, name, default_value)                                \
  inline type base_flag_val_##name = (default_value);                  \
  inline ::base::internal::FlagBuilder<type> base_flag_builder_##name( \
      &base_flag_val_##name, kCohort.data(), #name, (default_value));  \
  inline auto& base_flag_anchor_##name = base_flag_builder_##name

#define FLAG_LOOKUP(name) base_flag_val_##name

#define DECLARE_FLAG(cohort_name, type, name) \
  namespace base_flag_cohort_##cohort_name {  \
    extern type base_flag_val_##name;         \
  }                                           \
  inline const type& name =                   \
      ::base_flag_cohort_##cohort_name::base_flag_val_##name

#endif  // BASE_FLAGS_H_

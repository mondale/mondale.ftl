#ifndef CORE_STRINGS_H_
#define CORE_STRINGS_H_

#include <format>
#include <string>
#include <utility>

namespace core::strings {

template <typename... Args>
std::string Format(std::format_string<Args...> fmt, Args&&... args) {
  return std::format(fmt, std::forward<Args>(args)...);
}

}  // namespace core::strings

#endif  // #ifndef CORE_STRINGS_H_

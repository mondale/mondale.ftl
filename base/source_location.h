#ifndef BASE_SOURCE_LOCATION_H_
#define BASE_SOURCE_LOCATION_H_

#include <cstdint>
#include <string_view>

namespace base {

class SourceLocation {
 public:
  // Captures location metadata at the call site via compiler defaults.
  [[nodiscard]] static constexpr SourceLocation Current(
      const char* file = __builtin_FILE(),
      const char* function = __builtin_FUNCTION(), int line = __builtin_LINE(),
      int column = __builtin_COLUMN()) noexcept {
    SourceLocation loc;
    loc.file_ = file;
    loc.function_ = function;
    loc.line_ = line;
    loc.column_ = column;
    return loc;
  }

  constexpr SourceLocation() noexcept = default;

  [[nodiscard]] constexpr const char* file() const noexcept { return file_; }
  [[nodiscard]] constexpr const char* function() const noexcept {
    return function_;
  }
  [[nodiscard]] constexpr int line() const noexcept { return line_; }
  [[nodiscard]] constexpr int column() const noexcept { return column_; }

  // Strips absolute directory prefixes for cleaner test output.
  [[nodiscard]] constexpr std::string_view relative_file_name() const noexcept {
    std::string_view path{file_};
    auto pos = path.find_last_of("/\\");
    return (pos == std::string_view::npos) ? path : path.substr(pos + 1);
  }

 private:
  const char* file_ = "unknown";
  const char* function_ = "unknown";
  int line_ = 0;
  int column_ = 0;
};

}  // namespace base

#endif  // #ifndef BASE_SOURCE_LOCATION_H_

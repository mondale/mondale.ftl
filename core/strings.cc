#include <cstdint>
#include <string>
#include <string_view>

#include "core/result.h"
#include "core/strings.h"

namespace core::strings {
namespace {

// Check standard feature-test macro along with GCC/Clang and MSVC flags
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
constexpr bool kExceptionsEnabled = true;
#else
constexpr bool kExceptionsEnabled = false;
#endif

// Fail compilation if exceptions are enabled
static_assert(
    !kExceptionsEnabled,
    "This library must be compiled with -fno-exceptions! (std::format)");

constexpr char kHexDigits[] = "0123456789abcdef";

// Helper to determine UTF-8 sequence length based on leading byte.
// Returns 0 if the lead byte is invalid.
std::size_t Utf8SequenceLength(uint8_t lead) {
  if ((lead & 0x80) == 0x00) return 1;
  if ((lead & 0xE0) == 0xC0) return 2;
  if ((lead & 0xF0) == 0xE0) return 3;
  if ((lead & 0xF8) == 0xF0) return 4;
  return 0;
}

// Validates whether s starts with a well-formed UTF-8 code point of length
// `len`.
bool IsValidUtf8Sequence(std::string_view s, std::size_t len) {
  if (s.size() < len || len == 0) return false;

  const auto* u = reinterpret_cast<const uint8_t*>(s.data());
  if (len == 1) return u[0] <= 0x7F;

  if (len == 2) {
    if (u[0] < 0xC2) return false;  // Overlong encoding check
    return (u[1] & 0xC0) == 0x80;
  }

  if (len == 3) {
    if ((u[1] & 0xC0) != 0x80 || (u[2] & 0xC0) != 0x80) return false;
    // Overlong and surrogate checks
    if (u[0] == 0xE0 && u[1] < 0xA0) return false;
    if (u[0] == 0xED && u[1] >= 0xA0) return false;
    return true;
  }

  if (len == 4) {
    if ((u[1] & 0xC0) != 0x80 || (u[2] & 0xC0) != 0x80 ||
        (u[3] & 0xC0) != 0x80) {
      return false;
    }
    // Overlong and out-of-range checks
    if (u[0] == 0xF0 && u[1] < 0x90) return false;
    if (u[0] == 0xF4 && u[1] >= 0x90) return false;
    if (u[0] > 0xF4) return false;
    return true;
  }

  return false;
}

int HexDigitValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

}  // namespace

std::string EscapeString(std::string_view s) {
  std::string out;
  out.reserve(s.size());

  std::size_t i = 0;
  while (i < s.size()) {
    const uint8_t byte = static_cast<uint8_t>(s[i]);
    const std::size_t utf8_len = Utf8SequenceLength(byte);

    // Valid multi-byte UTF-8 sequence: preserve as-is.
    if (utf8_len > 1 && IsValidUtf8Sequence(s.substr(i), utf8_len)) {
      out.append(s.substr(i, utf8_len));
      i += utf8_len;
      continue;
    }

    // Handle single-byte ASCII or invalid/ill-formed UTF-8 byte.
    switch (s[i]) {
      case '"':
        out.append("\\\"");
        break;
      case '\\':
        out.append("\\\\");
        break;
      case '\b':
        out.append("\\b");
        break;
      case '\f':
        out.append("\\f");
        break;
      case '\n':
        out.append("\\n");
        break;
      case '\r':
        out.append("\\r");
        break;
      case '\t':
        out.append("\\t");
        break;
      default:
        // Escape ASCII control chars or non-ASCII bytes that are not part
        // of a valid UTF-8 sequence in \xXX hex format.
        if (byte < 0x20 || byte >= 0x7F) {
          out.append("\\x");
          out.push_back(kHexDigits[(byte >> 4) & 0x0F]);
          out.push_back(kHexDigits[byte & 0x0F]);
        } else {
          out.push_back(s[i]);
        }
        break;
    }
    ++i;
  }

  return out;
}

ResultOr<std::string> UnescapeString(std::string_view s) {
  std::string out;
  out.reserve(s.size());

  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] != '\\') {
      out.push_back(s[i]);
      continue;
    }

    if (i + 1 >= s.size()) {
      return Result(Code::kInvalidArgument,
                    "Trailing backslash in escape sequence");
    }

    ++i;
    switch (s[i]) {
      case '"':
        out.push_back('"');
        break;
      case '\\':
        out.push_back('\\');
        break;
      case '/':
        out.push_back('/');
        break;
      case 'b':
        out.push_back('\b');
        break;
      case 'f':
        out.push_back('\f');
        break;
      case 'n':
        out.push_back('\n');
        break;
      case 'r':
        out.push_back('\r');
        break;
      case 't':
        out.push_back('\t');
        break;
      case 'x': {
        if (i + 2 >= s.size()) {
          return Result(Code::kInvalidArgument, "Incomplete \\x hex escape");
        }
        const int h1 = HexDigitValue(s[i + 1]);
        const int h2 = HexDigitValue(s[i + 2]);
        if (h1 < 0 || h2 < 0) {
          return Result(Code::kInvalidArgument,
                        "Invalid hex digits in \\x escape");
        }
        out.push_back(static_cast<char>((h1 << 4) | h2));
        i += 2;
        break;
      }
      default:
        return Result(Code::kInvalidArgument, "Unknown escape sequence");
    }
  }

  return out;
}

}  // namespace core::strings

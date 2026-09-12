#include <charconv>

#include "capsule/text/text_parser.h"

namespace capsule::text {

TextParser::TextParser(std::string_view src)
    : lexer_(src), current_(lexer_.NextToken()) {}

bool TextParser::IsAtEnd() const { return current_.type == Token::Type::kEof; }

bool TextParser::Check(Token::Type type) const { return current_.type == type; }

Result TextParser::Match(Token::Type type) {
  if (Check(type)) {
    Advance();
    return Result::Ok();
  }
  return Result(Code::kError, "Unexpected token " + current_.ToString());
}

ResultOr<std::string> TextParser::ExpectIdentifier() {
  if (current_.type != Token::Type::kIdentifier) {
    return Result(Code::kError,
                  "Expected identifier at " + current_.ToString());
  }
  std::string name = current_.text;
  Advance();
  return name;
}

ResultOr<std::string> TextParser::ExpectString() {
  if (current_.type != Token::Type::kStringLiteral) {
    return Result(Code::kError,
                  "Expected string literal, found " + current_.ToString());
  }
  std::string val = current_.text;
  Advance();
  return val;
}

ResultOr<std::string> TextParser::ExpectNumber() {
  if (current_.type != Token::Type::kNumberLiteral) {
    return Result(Code::kError,
                  "Expected number literal, found " + current_.ToString());
  }
  std::string text = current_.text;
  Advance();
  return text;
}

ResultOr<uint8_t> TextParser::ParseU8() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  unsigned long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val > UINT8_MAX) {
    return Result(Code::kError, "Invalid or out-of-range u8 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<uint8_t>(val);
}

ResultOr<int8_t> TextParser::ParseI8() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val < INT8_MIN || val > INT8_MAX) {
    return Result(Code::kError, "Invalid or out-of-range i8 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<int8_t>(val);
}

ResultOr<uint16_t> TextParser::ParseU16() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  unsigned long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val > UINT16_MAX) {
    return Result(Code::kError, "Invalid or out-of-range u16 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<uint16_t>(val);
}

ResultOr<int16_t> TextParser::ParseI16() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val < INT16_MIN || val > INT16_MAX) {
    return Result(Code::kError, "Invalid or out-of-range i16 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<int16_t>(val);
}

ResultOr<uint32_t> TextParser::ParseU32() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  unsigned long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val > UINT32_MAX) {
    return Result(Code::kError, "Invalid or out-of-range u32 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<uint32_t>(val);
}

ResultOr<int32_t> TextParser::ParseI32() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  long val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc() || val < INT32_MIN || val > INT32_MAX) {
    return Result(Code::kError, "Invalid or out-of-range i32 value at line " +
                                    std::to_string(current_.line));
  }
  return static_cast<int32_t>(val);
}

ResultOr<uint64_t> TextParser::ParseU64() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  uint64_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc()) {
    return Result(Code::kError,
                  "Invalid u64 value at line " + std::to_string(current_.line));
  }
  return val;
}

ResultOr<int64_t> TextParser::ParseI64() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  int64_t val = 0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc()) {
    return Result(Code::kError,
                  "Invalid i64 value at line " + std::to_string(current_.line));
  }
  return val;
}

ResultOr<float> TextParser::ParseF32() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  float val = 0.0f;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc()) {
    return Result(Code::kError,
                  "Invalid f32 value at line " + std::to_string(current_.line));
  }
  return val;
}

ResultOr<double> TextParser::ParseF64() {
  TRY_ASSIGN(std::string s, ExpectNumber());
  double val = 0.0;
  auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
  if (ec != std::errc()) {
    return Result(Code::kError,
                  "Invalid f64 value at line " + std::to_string(current_.line));
  }
  return val;
}

ResultOr<std::string> TextParser::ParseString() {
  if (Check(Token::Type::kStringLiteral)) {
    return ExpectString();
  }
  return ExpectIdentifier();
}

ResultOr<std::vector<std::string>> TextParser::ParseStringVector() {
  std::vector<std::string> values;
  do {
    if (Check(Token::Type::kComma)) {
      Advance();  // consume comma separator
    }
    if (Check(Token::Type::kStringLiteral) || Check(Token::Type::kIdentifier)) {
      TRY_ASSIGN(std::string s, ParseString());
      values.push_back(std::move(s));
    } else {
      break;
    }
  } while (Check(Token::Type::kComma));
  return values;
}

ResultOr<TextParser> TextParser::ExtractBlockParser() {
  if (!Check(Token::Type::kLBrace)) {
    return Result(Code::kError, "Expected '{' for capsule block at line " +
                                    std::to_string(current_.line));
  }

  // Scan source text starting from current lexer position to find matching '}'
  std::string_view src = lexer_.source();
  size_t start_pos = lexer_.position();

  // We already consumed '{' in terms of token, but lexer position is past it.
  // Let's find the exact range of the block contents by scanning
  // characters/braces.
  size_t current_idx = start_pos;
  int brace_depth = 1;

  while (current_idx < src.size() && brace_depth > 0) {
    char ch = src[current_idx];
    if (ch == '{') {
      brace_depth++;
    } else if (ch == '}') {
      brace_depth--;
      if (brace_depth == 0) {
        break;
      }
    } else if (ch == '"') {
      // Skip string literals to avoid counting braces inside strings
      current_idx++;
      while (current_idx < src.size() && src[current_idx] != '"') {
        if (src[current_idx] == '\\' && current_idx + 1 < src.size()) {
          current_idx++;
        }
        current_idx++;
      }
    }
    current_idx++;
  }

  if (brace_depth > 0) {
    return Result(Code::kError, "Unterminated capsule block starting at line " +
                                    std::to_string(current_.line));
  }

  // Extract inner content view (excluding the final '}')
  std::string_view block_content =
      src.substr(start_pos, current_idx - start_pos);

  // Advance our own lexer past the entire block (including the closing '}')
  // We can re-sync our lexer by advancing until current_idx + 1
  while (lexer_.position() <= current_idx && !IsAtEnd()) {
    Advance();
  }

  return TextParser(block_content);
}

ResultOr<TextParser> TextParser::EnterCapsule() {
  TRY(Match(Token::Type::kLBrace));
  // Back up lexer position by 1 token/char if needed, or extract block starting
  // from current position Since Match consumed '{', lexer position is right
  // after '{'.
  return ExtractBlockParser();
}

ResultOr<TextParser> TextParser::EnterVectorCapsule() { return EnterCapsule(); }

int TextParser::current_line() const { return current_.line; }

void TextParser::Advance() { current_ = lexer_.NextToken(); }

}  // namespace capsule::text

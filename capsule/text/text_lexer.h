#ifndef CAPSULE_TEXT_LEXER_H_
#define CAPSULE_TEXT_LEXER_H_

#include <string>
#include <string_view>

namespace capsule::text {

struct Token {
  enum class Type {
    kEof,
    kUnknown,
    kIdentifier,
    kStringLiteral,
    kNumberLiteral,
    kLBrace,    // {
    kRBrace,    // }
    kLBracket,  // [
    kRBracket,  // ]
    kComma,     // ,
    kColon,     // :
    kCapsule    // capsule
  };

  Type type;
  std::string text;
  int line;

  std::string ToString() const;
};

class TextLexer {
 public:
  explicit TextLexer(std::string_view sv);

  Token NextToken();
  int line() const;
  size_t position() const;
  std::string_view source() const;

 private:
  void SkipWhitespace();

  std::string_view src_;
  size_t pos_;
  int line_;
};

}  // namespace capsule::text

#endif  // #ifndef CAPSULE_TEXT_LEXER_H_

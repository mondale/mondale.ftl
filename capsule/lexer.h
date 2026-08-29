#ifndef CAPSULE_LEXER_H_
#define CAPSULE_LEXER_H_

#include <string>
#include <string_view>

#include "core/vocabulary.h"

namespace capsule {

struct Token {
  enum class Type {
    kNamespace,
    kCapsule,
    kIdentifier,
    kStringLiteral,
    kNumberLiteral,
    kAt,
    kLBrace,
    kRBrace,
    kLBracket,
    kRBracket,
    kLParen,
    kRParen,
    kComma,
    kEof,
    kUnknown
  };
  Type tp;
  std::string val;
  int line;
};

class Lexer {
 public:
  explicit Lexer(std::string_view sv);
  Token NextToken();
  int line() const;

 private:
  void SkipWhitespace();

  std::string_view src_;
  size_t pos_;
  int line_;
};

}  // namespace capsule

#endif  // CAPSULE_LEXER_H_

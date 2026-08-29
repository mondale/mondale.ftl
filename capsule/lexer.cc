#include <cctype>

#include "capsule/lexer.h"

namespace capsule {

Lexer::Lexer(std::string_view sv) : src_(sv), pos_(0), line_(1) {}

Token Lexer::NextToken() {
  SkipWhitespace();
  if (pos_ >= src_.size()) {
    return {Token::Type::kEof, "", line_};
  }

  int token_line = line_;
  char ch = src_[pos_];
  if (ch == '@') {
    pos_++;
    return {Token::Type::kAt, "@", token_line};
  }
  if (ch == '{') {
    pos_++;
    return {Token::Type::kLBrace, "{", token_line};
  }
  if (ch == '}') {
    pos_++;
    return {Token::Type::kRBrace, "}", token_line};
  }
  if (ch == '[') {
    pos_++;
    return {Token::Type::kLBracket, "[", token_line};
  }
  if (ch == ']') {
    pos_++;
    return {Token::Type::kRBracket, "]", token_line};
  }
  if (ch == '(') {
    pos_++;
    return {Token::Type::kLParen, "(", token_line};
  }
  if (ch == ')') {
    pos_++;
    return {Token::Type::kRParen, ")", token_line};
  }
  if (ch == ',') {
    pos_++;
    return {Token::Type::kComma, ",", token_line};
  }

  if (ch == '"') {
    pos_++;
    size_t start = pos_;
    while (pos_ < src_.size() && src_[pos_] != '"') {
      if (src_[pos_] == '\n') line_++;
      pos_++;
    }
    std::string s(src_.substr(start, pos_ - start));
    if (pos_ < src_.size()) pos_++;  // skip closing quote
    return {Token::Type::kStringLiteral, s, token_line};
  }

  if (std::isalpha(ch) || ch == '_') {
    size_t start = pos_;
    while (pos_ < src_.size() &&
           (std::isalnum(src_[pos_]) || src_[pos_] == '_' ||
            src_[pos_] == '<' || src_[pos_] == '>')) {
      pos_++;
    }
    std::string id(src_.substr(start, pos_ - start));
    if (id == "namespace") return {Token::Type::kNamespace, id, token_line};
    if (id == "capsule") return {Token::Type::kCapsule, id, token_line};
    return {Token::Type::kIdentifier, id, token_line};
  }

  if (std::isdigit(ch)) {
    size_t start = pos_;
    while (pos_ < src_.size() && std::isdigit(src_[pos_])) {
      pos_++;
    }
    return {Token::Type::kNumberLiteral,
            std::string(src_.substr(start, pos_ - start)), token_line};
  }

  pos_++;
  return {Token::Type::kUnknown, std::string(1, ch), token_line};
}

int Lexer::line() const { return line_; }

void Lexer::SkipWhitespace() {
  while (pos_ < src_.size()) {
    char ch = src_[pos_];
    if (ch == '\n') {
      line_++;
      pos_++;
    } else if (std::isspace(ch)) {
      pos_++;
    } else if (pos_ + 1 < src_.size() && src_[pos_] == '/' &&
               src_[pos_ + 1] == '/') {
      while (pos_ < src_.size() && src_[pos_] != '\n') {
        pos_++;
      }
    } else {
      break;
    }
  }
}

}  // namespace capsule

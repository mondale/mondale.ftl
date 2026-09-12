#include <cctype>

#include "capsule/text/text_lexer.h"
#include "core/vocabulary.h"

namespace capsule::text {
namespace {

std::string TypeString(Token::Type t) {
  switch (t) {
    case Token::Type::kEof:
      return "kEof";
    case Token::Type::kUnknown:
      return "kUnknown";
    case Token::Type::kIdentifier:
      return "kIdentifier";
    case Token::Type::kStringLiteral:
      return "kStringLiteral";
    case Token::Type::kNumberLiteral:
      return "kNumberLiteral";
    case Token::Type::kLBrace:
      return "kLBrace";
    case Token::Type::kRBrace:
      return "kRBrace";
    case Token::Type::kLBracket:
      return "kLBracket";
    case Token::Type::kRBracket:
      return "kRBracket";
    case Token::Type::kComma:
      return "kComma";
    case Token::Type::kColon:
      return "kColon";
    case Token::Type::kCapsule:
      return "kCapsule";
  }
}

}  // namespace

std::string Token::ToString() const {
  return strings::Format("Line {}: Type[{}] Text[{}]", line, TypeString(type),
                         text);
}

TextLexer::TextLexer(std::string_view sv) : src_(sv), pos_(0), line_(1) {}

Token TextLexer::NextToken() {
  SkipWhitespace();
  if (pos_ >= src_.size()) {
    return {Token::Type::kEof, "", line_};
  }

  int token_line = line_;
  char ch = src_[pos_];

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
  if (ch == ',') {
    pos_++;
    return {Token::Type::kComma, ",", token_line};
  }
  if (ch == ':') {
    pos_++;
    return {Token::Type::kColon, ":", token_line};
  }

  if (ch == '"') {
    // TODO - this is fragile as heck.
    pos_++;
    std::string s;
    bool escaped = false;

    while (pos_ < src_.size()) {
      char curr = src_[pos_];
      if (curr == '\n') {
        line_++;
      }
      if (escaped) {
        switch (curr) {
          case 'n':
            s.push_back('\n');
            break;
          case 't':
            s.push_back('\t');
            break;
          case 'r':
            s.push_back('\r');
            break;
          case '\\':
            s.push_back('\\');
            break;
          case '"':
            s.push_back('"');
            break;
          default:
            // Preserve unrecognized escapes as literal backslash + char
            s.push_back('\\');
            s.push_back(curr);
            break;
        }
        escaped = false;
      } else if (curr == '\\') {
        escaped = true;
      } else if (curr == '"') {
        break;
      } else {
        s.push_back(curr);
      }
      pos_++;
    }

    if (pos_ < src_.size()) {
      pos_++;  // skip closing quote
    }
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
    if (id == "capsule") return {Token::Type::kCapsule, id, token_line};
    return {Token::Type::kIdentifier, id, token_line};
  }

  if (std::isdigit(ch) || ch == '-' || ch == '+') {
    // Handle potential signs for numbers if needed, or standard digit
    // sequence
    size_t start = pos_;
    if (ch == '-' || ch == '+') pos_++;
    bool has_dot = false;
    while (pos_ < src_.size() &&
           (std::isdigit(src_[pos_]) || (!has_dot && src_[pos_] == '.'))) {
      if (src_[pos_] == '.') has_dot = true;
      pos_++;
    }
    return {Token::Type::kNumberLiteral,
            std::string(src_.substr(start, pos_ - start)), token_line};
  }

  pos_++;
  return {Token::Type::kUnknown, std::string(1, ch), token_line};
}

int TextLexer::line() const { return line_; }

size_t TextLexer::position() const { return pos_; }

std::string_view TextLexer::source() const { return src_; }

void TextLexer::SkipWhitespace() {
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

}  // namespace capsule::text

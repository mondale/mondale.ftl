#ifndef CAPSULE_TEXT_PARSER_H_
#define CAPSULE_TEXT_PARSER_H_

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "capsule/text/text_lexer.h"
#include "core/vocabulary.h"

namespace capsule::text {

class TextParser {
 public:
  // Initializes the parser with the source text view.
  explicit TextParser(std::string_view src);

  const Token& CurrentToken() const { return current_; }

  // Returns true if the parser has reached the end of the input stream.
  bool IsAtEnd() const;

  // Returns true if the current token matches the specified type.
  bool Check(Token::Type type) const;

  // Consumes the current token if it matches the specified type, otherwise
  // returns an error.
  Result Match(Token::Type type);

  // Consumes and returns the current token as an identifier string.
  ResultOr<std::string> ExpectIdentifier();

  // Consumes and returns the current token as a string literal.
  ResultOr<std::string> ExpectString();

  // Consumes and returns the current token as a numeric literal string.
  ResultOr<std::string> ExpectNumber();

  // Parses and returns an unsigned 8-bit integer.
  ResultOr<uint8_t> ParseU8();

  // Parses and returns a signed 8-bit integer.
  ResultOr<int8_t> ParseI8();

  // Parses and returns an unsigned 16-bit integer.
  ResultOr<uint16_t> ParseU16();

  // Parses and returns a signed 16-bit integer.
  ResultOr<int16_t> ParseI16();

  // Parses and returns an unsigned 32-bit integer.
  ResultOr<uint32_t> ParseU32();

  // Parses and returns a signed 32-bit integer.
  ResultOr<int32_t> ParseI32();

  // Parses and returns an unsigned 64-bit integer.
  ResultOr<uint64_t> ParseU64();

  // Parses and returns a signed 64-bit integer.
  ResultOr<int64_t> ParseI64();

  // Parses and returns a 32-bit floating-point number.
  ResultOr<float> ParseF32();

  // Parses and returns a 64-bit floating-point number.
  ResultOr<double> ParseF64();

  // Parses and returns a string value (identifier or string literal).
  ResultOr<std::string> ParseString();

  // Parses and returns a comma-separated vector of string values.
  ResultOr<std::vector<std::string>> ParseStringVector();

  // Enters a nested capsule block, returning a parser initialized exclusively
  // for its contents.
  ResultOr<TextParser> EnterCapsule();

  // Enters the next capsule block within a vector sequence, returning its
  // parser.
  ResultOr<TextParser> EnterVectorCapsule();

  // Returns the line number of the current token.
  int current_line() const;

 private:
  void Advance();
  ResultOr<TextParser> ExtractBlockParser();

  TextLexer lexer_;
  Token current_;
};

}  // namespace capsule::text

#endif  // #ifndef CAPSULE_TEXT_PARSER_H_

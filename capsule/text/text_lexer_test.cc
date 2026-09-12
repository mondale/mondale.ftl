#include "capsule/text/text_lexer.h"
#include "testing/testing.h"

namespace capsule::text {

TEST(TextLexerTest_EscapedStrings) {
  std::string_view s = "\"[\\\"Hello, world!\\\"]\"";
  capsule::text::TextLexer l(s);
  auto token = l.NextToken();
  ASSERT_TRUE(Token::Type::kStringLiteral == token.type)
      << static_cast<int>(token.type);
  ASSERT_EQ(token.text, "[\"Hello, world!\"]");
}

TEST(TextLexerTest_EscapedStringsGross) {
  std::string_view s = "\"2`%<@0MH!?0\\\"|;JDX!?0lmC$6Q\"";
  capsule::text::TextLexer l(s);
  auto token = l.NextToken();
  ASSERT_TRUE(Token::Type::kStringLiteral == token.type)
      << static_cast<int>(token.type);
  std::string_view expected = "2`%<@0MH!?0\"|;JDX!?0lmC$6Q";
  ASSERT_EQ(token.text, expected);
}

}  // namespace capsule::text

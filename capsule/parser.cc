#include <sstream>

#include "capsule/parser.h"

namespace capsule {

class Parser::Impl {
 public:
  Impl(std::string_view sv, std::string_view fn)
      : lex_(sv), tok_(lex_.NextToken()), filename_(fn) {}

  ResultOr<CapsuleFile> ParseFile() {
    CapsuleFile file;

    if (tok_.tp != Token::Type::kNamespace) {
      return MakeError(
          "Expected 'namespace' declaration at the top of the file");
    }
    tok_ = lex_.NextToken();

    if (tok_.tp != Token::Type::kIdentifier) {
      return MakeError("Expected namespace name");
    }
    file.namespace_name = tok_.val;
    tok_ = lex_.NextToken();

    while (tok_.tp != Token::Type::kEof) {
      if (tok_.tp != Token::Type::kCapsule) {
        return MakeError("Expected 'capsule' keyword");
      }
      TRY_ASSIGN(auto cp, ParseCapsule());
      file.capsules.push_back(std::move(cp));
    }

    return file;
  }

 private:
  Result MakeError(std::string_view msg) {
    std::string formatted =
        strings::Format("{}:{}: Error: {}", filename_, tok_.line, msg);
    return Result(Code::kError, formatted);
  }

  ResultOr<Capsule> ParseCapsule() {
    tok_ = lex_.NextToken();  // consume 'capsule'

    if (tok_.tp != Token::Type::kIdentifier) {
      return MakeError("Expected capsule name");
    }
    Capsule cp;
    cp.name = tok_.val;
    tok_ = lex_.NextToken();

    if (tok_.tp != Token::Type::kLBrace) {
      return MakeError("Expected '{' after capsule name");
    }
    tok_ = lex_.NextToken();

    while (tok_.tp != Token::Type::kRBrace && tok_.tp != Token::Type::kEof) {
      std::vector<Attribute> attrs;
      while (tok_.tp == Token::Type::kAt) {
        TRY_ASSIGN(auto at, ParseAttribute());
        attrs.push_back(std::move(at));
      }

      if (tok_.tp != Token::Type::kIdentifier) {
        return MakeError("Expected field type");
      }
      std::string tp = tok_.val;
      tok_ = lex_.NextToken();

      if (tok_.tp != Token::Type::kIdentifier) {
        return MakeError("Expected field name");
      }
      std::string nm = tok_.val;
      tok_ = lex_.NextToken();

      cp.fields.push_back(
          Field{std::move(tp), std::move(nm), std::move(attrs)});
    }

    if (tok_.tp != Token::Type::kRBrace) {
      return MakeError("Expected '}' at end of capsule");
    }
    tok_ = lex_.NextToken();

    return cp;
  }

  ResultOr<Attribute> ParseAttribute() {
    tok_ = lex_.NextToken();  // skip '@'
    if (tok_.tp != Token::Type::kIdentifier) {
      return MakeError("Expected attribute name after '@'");
    }
    std::string an = tok_.val;
    tok_ = lex_.NextToken();

    std::string val = "";
    if (tok_.tp == Token::Type::kLParen) {
      tok_ = lex_.NextToken();
      if (tok_.tp == Token::Type::kStringLiteral ||
          tok_.tp == Token::Type::kNumberLiteral ||
          tok_.tp == Token::Type::kLBracket) {
        val = tok_.val;
        tok_ = lex_.NextToken();
      }
      if (tok_.tp != Token::Type::kRParen) {
        return MakeError("Expected ')' after attribute value");
      }
      tok_ = lex_.NextToken();
    }
    return Attribute{std::move(an), std::move(val)};
  }

  Lexer lex_;
  Token tok_;
  std::string filename_;
};

Parser::Parser(std::string_view src, std::string_view filename)
    : impl_(std::make_unique<Impl>(src, filename)) {}
Parser::~Parser() {}

ResultOr<CapsuleFile> Parser::Parse() { return impl_->ParseFile(); }

}  // namespace capsule

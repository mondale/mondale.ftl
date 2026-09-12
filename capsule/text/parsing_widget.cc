#include "capsule/text/parsing_widget.h"

namespace capsule::text {
namespace {

template <typename T>
T* FindOrNull(std::map<std::string_view, T*>* m, std::string_view n) {
  const auto i = m->find(n);
  if (i == m->end()) return nullptr;
  return i->second;
}

}  // namespace

Result ParsingWidget::ParsePrimitive(std::string_view n,
                                     capsule::text::TextParser* p) {
  if (auto* x = FindOrNull(&u64m_, n)) {
    TRY_ASSIGN(*x, p->ParseU64());
  } else if (auto* x = FindOrNull(&i64m_, n)) {
    TRY_ASSIGN(*x, p->ParseI64());
  } else if (auto* x = FindOrNull(&f64m_, n)) {
    TRY_ASSIGN(*x, p->ParseF64());
  } else if (auto* x = FindOrNull(&u32m_, n)) {
    TRY_ASSIGN(*x, p->ParseU32());
  } else if (auto* x = FindOrNull(&i32m_, n)) {
    TRY_ASSIGN(*x, p->ParseI32());
  } else if (auto* x = FindOrNull(&f32m_, n)) {
    TRY_ASSIGN(*x, p->ParseF32());
  } else if (auto* x = FindOrNull(&u16m_, n)) {
    TRY_ASSIGN(*x, p->ParseU16());
  } else if (auto* x = FindOrNull(&i16m_, n)) {
    TRY_ASSIGN(*x, p->ParseI16());
  } else if (auto* x = FindOrNull(&u8m_, n)) {
    TRY_ASSIGN(*x, p->ParseU8());
  } else if (auto* x = FindOrNull(&i8m_, n)) {
    TRY_ASSIGN(*x, p->ParseI8());
  } else if (auto* x = FindOrNull(&bm_, n)) {
    TRY_ASSIGN(const auto v, p->ParseU8());
    *x = static_cast<bool>(v);
  } else if (auto* x = FindOrNull(&string_m_, n)) {
    TRY_ASSIGN(*x, p->ParseString());
  } else {
    const auto& t = p->CurrentToken();
    return core::CapsuleFatalError(strings::Format(
        "Line {}: No parse possible for [{}]: [{}].", t.line, n, t.text));
  }
  return Result::Ok();
}

Result ParsingWidget::ParseSubcapsule(std::string_view n,
                                      capsule::text::TextParser* p) {
  const auto i = capsule_m_.find(n);
  if (capsule_m_.end() == i) {
    const auto& t = p->CurrentToken();
    return core::CapsuleFatalError(strings::Format(
        "Line {}: No capsule known for [{}]: [{}].", t.line, n, t.text));
  }
  return (i->second)(p);
}

Result ParsingWidget::ParseStringVector(capsule::text::TextParser* p,
                                        std::vector<std::string>* v) {
  TRY(p->Match(Token::Type::kLBrace));
  v->clear();
  while (p->Check(Token::Type::kStringLiteral)) {
    TRY_ASSIGN(v->emplace_back(), p->ExpectString());

    // Commas are optional in the syntax.
    if (p->Check(Token::Type::kComma)) {
      TRY(p->Match(Token::Type::kComma));
    }
  }
  return p->Match(Token::Type::kRBrace);
}

Result ParsingWidget::ParseVector(std::string_view n,
                                  capsule::text::TextParser* p) {
  // If this is a string vector, we handle locally.
  const auto i = string_vector_m_.find(n);
  if (string_vector_m_.end() != i) {
    // It's a string vector
    return ParseStringVector(p, i->second);
  }

  const auto j = capsule_vector_m_.find(n);
  if (capsule_vector_m_.end() == j) {
    const auto& t = p->CurrentToken();
    return core::CapsuleFatalError(
        strings::Format("Line {}: No vector type known for [{}].", t.line, n));
  }

  // It's a capsule vector. Each invocation of the fn adds a new capsule to the
  // vector and invokes Parse on that capsule. So this framing needs to strip
  // the leading brace and then invoke the ParseFn.
  while (p->Check(Token::Type::kLBrace)) {
    TRY(p->Match(Token::Type::kLBrace));
    TRY((j->second)(p));
  }
  return Result::Ok();
}

Result ParsingWidget::ParseFrom(::capsule::text::TextParser* p) {
  // Begin parsing the input stream at the start of a given capsule.
  while (!p->IsAtEnd()) {
    if (p->Check(Token::Type::kRBrace)) {
      return p->Match(Token::Type::kRBrace);
      break;  // that's end of our capsule.
    }

    // Everything at this point should be an identifier.
    TRY_ASSIGN(std::string field_name, p->ExpectIdentifier());

    // Depending on what comes next, we go into a more specific parsing
    // sequence...
    // * If it's a {, we're parsing a subcapsule.
    if (p->Check(Token::Type::kLBrace)) {
      TRY(p->Match(Token::Type::kLBrace));
      TRY(ParseSubcapsule(field_name, p));
      continue;
    }

    // * If it's a [, we're parsing a vector of something.
    if (p->Check(Token::Type::kLBracket)) {
      TRY(p->Match(Token::Type::kLBracket));
      TRY(p->Match(Token::Type::kRBracket));
      TRY(ParseVector(field_name, p));
      continue;
    }

    // * If it's a :, we're parsing a primitive.
    if (p->Check(Token::Type::kColon)) {
      TRY(p->Match(Token::Type::kColon));
      TRY(ParsePrimitive(field_name, p));
      continue;
    }

    // ... anything else is invalid.
    return core::InvalidArgumentError(
        strings::Format("Not expecting token [{}] at line [{}].",
                        p->CurrentToken().text, p->current_line()));
  }
  return Result::Ok();
}

void ParsingWidget::Add(std::string_view n, bool* bp) { bm_[n] = bp; }
void ParsingWidget::Add(std::string_view n, uint8_t* u8p) { u8m_[n] = u8p; }
void ParsingWidget::Add(std::string_view n, int8_t* i8p) { i8m_[n] = i8p; }
void ParsingWidget::Add(std::string_view n, uint16_t* u16p) { u16m_[n] = u16p; }
void ParsingWidget::Add(std::string_view n, int16_t* i16p) { i16m_[n] = i16p; }
void ParsingWidget::Add(std::string_view n, uint32_t* u32p) { u32m_[n] = u32p; }
void ParsingWidget::Add(std::string_view n, int32_t* i32p) { i32m_[n] = i32p; }
void ParsingWidget::Add(std::string_view n, float* f32p) { f32m_[n] = f32p; }
void ParsingWidget::Add(std::string_view n, uint64_t* u64p) { u64m_[n] = u64p; }
void ParsingWidget::Add(std::string_view n, int64_t* i64p) { i64m_[n] = i64p; }
void ParsingWidget::Add(std::string_view n, double* f64p) { f64m_[n] = f64p; }
void ParsingWidget::Add(std::string_view n, std::string* sp) {
  string_m_[n] = sp;
}

void ParsingWidget::AddStringVector(std::string_view n,
                                    std::vector<std::string>* vsp) {
  string_vector_m_[n] = vsp;
}

void ParsingWidget::AddCapsule(std::string_view n, ParseFn fn) {
  capsule_m_[n] = std::move(fn);
}

void ParsingWidget::AddCapsuleVector(std::string_view n, ParseFn fn) {
  capsule_vector_m_[n] = std::move(fn);
}

}  // namespace capsule::text

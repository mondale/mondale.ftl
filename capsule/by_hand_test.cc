#include <map>
#include <random>
#include <vector>
// TODO - need randomness support in runtime

#include "base/logging.h"
#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/size_builder.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "capsule/text/text_parser.h"
#include "testing/testing.h"

using testing::IsOk;

namespace {

std::string Spaces(int amount) { return std::string(amount, ' '); }

struct SubSubM;
struct SubSubV;

struct SubSubBase {
  using MaterializedType = SubSubM;
  using ViewType = SubSubV;

  static constexpr uint32_t kFieldCount = 3;
  static constexpr core::CRC32C b1_FieldHash = core::CRC32C(31);
  static constexpr core::CRC32C i1_FieldHash = core::CRC32C(32);
  static constexpr core::CRC32C s1_FieldHash = core::CRC32C(33);
  static constexpr bool b1_Default = false;
  static constexpr int32_t i1_Default = 77;
  static constexpr std::string s1_Default = "Oooh";
  static constexpr int b1_Index = 0;
  static constexpr int i1_Index = 1;
  static constexpr int s1_Index = 2;

  bool has_b1() const { return has() && has_[b1_Index]; }
  bool has_i1() const { return has() && has_[i1_Index]; }
  bool has_s1() const { return has() && has_[s1_Index]; }
  bool has() const { return !has_.empty(); }

  std::vector<bool> has_;
};

struct SubSubM final : public SubSubBase {
  bool b1;
  int32_t i1;
  std::string s1;

  size_t ComputeStorageSize() const;
  void Encode(::capsule::Encoder* e) const;
  Result Decode(::capsule::Decoder* d);
  std::string ToString(int indent = 0) const;
  Result ParseFrom(::capsule::text::TextParser* p);
};

using capsule::text::Token;
class ParsingWidget final {
 public:
  using ParseFn = std::function<Result(capsule::text::TextParser* p)>;

  void Add(std::string_view n, bool* bp);
  void Add(std::string_view n, uint8_t* u8p);
  void Add(std::string_view n, int8_t* i8p);
  void Add(std::string_view n, uint16_t* u16p);
  void Add(std::string_view n, int16_t* i16p);
  void Add(std::string_view n, uint32_t* u32p);
  void Add(std::string_view n, int32_t* i32p);
  void Add(std::string_view n, float* f32p);
  void Add(std::string_view n, uint64_t* u64p);
  void Add(std::string_view n, int64_t* i64p);
  void Add(std::string_view n, double* f64p);
  void Add(std::string_view n, std::string* sp);
  void AddStringVector(std::string_view n, std::vector<std::string>* vsp);

  // The target of this ParseFn is Parse on the nested capsule.
  void AddCapsule(std::string_view n, ParseFn fn);

  // The target of this ParseFn is a method that invokes emplace_back on the
  // target vector then invokes Parse on the nested capsule.
  void AddCapsuleVector(std::string_view n, ParseFn fn);

  Result ParseFrom(::capsule::text::TextParser* p);

 private:
  Result ParseSubcapsule(std::string_view n, capsule::text::TextParser* p);
  Result ParseVector(std::string_view n, capsule::text::TextParser* p);
  Result ParsePrimitive(std::string_view n, capsule::text::TextParser* p);
  Result ParseStringVector(capsule::text::TextParser* p,
                           std::vector<std::string>* v);

  std::map<std::string_view, bool*> bm_;
  std::map<std::string_view, uint8_t*> u8m_;
  std::map<std::string_view, int8_t*> i8m_;
  std::map<std::string_view, uint16_t*> u16m_;
  std::map<std::string_view, int16_t*> i16m_;
  std::map<std::string_view, uint32_t*> u32m_;
  std::map<std::string_view, int32_t*> i32m_;
  std::map<std::string_view, float*> f32m_;
  std::map<std::string_view, uint64_t*> u64m_;
  std::map<std::string_view, int64_t*> i64m_;
  std::map<std::string_view, double*> f64m_;
  std::map<std::string_view, std::string*> string_m_;
  std::map<std::string_view, std::vector<std::string>*> string_vector_m_;
  std::map<std::string_view, ParseFn> capsule_m_;
  std::map<std::string_view, ParseFn> capsule_vector_m_;
};

template <typename T>
T* FindOrNull(std::map<std::string_view, T*>* m, std::string_view n) {
  const auto i = m->find(n);
  if (i == m->end()) return nullptr;
  return i->second;
}

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

Result SubSubM::ParseFrom(::capsule::text::TextParser* p) {
  ParsingWidget widget;
  widget.Add("b1", &b1);
  widget.Add("i1", &i1);
  widget.Add("s1", &s1);
  return widget.ParseFrom(p);
}

std::string SubSubM::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_b1())
    oss << Spaces(indent) << "b1" << ": " << b1 << std::endl;
  if (!has() || has_i1())
    oss << Spaces(indent) << "i1" << ": " << i1 << std::endl;
  if (!has() || has_s1())
    oss << Spaces(indent) << "s1" << ": \"" << strings::EscapeString(s1) << "\""
        << std::endl;
  return oss.str();
}

size_t SubSubM::ComputeStorageSize() const {
  ::capsule::SizeBuilder sb;
  sb.Add(b1);
  sb.Add(i1);
  sb.Add(s1);
  return sb.Build();
}

void SubSubM::Encode(::capsule::Encoder* e) const {
  e->Add(b1_FieldHash, b1);
  e->Add(i1_FieldHash, i1);
  e->Add(s1_FieldHash, s1);
}

Result SubSubM::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(d->Find(b1_FieldHash, &b1, b1_Default, has_[b1_Index]));
  ret.Incorporate(d->Find(i1_FieldHash, &i1, i1_Default, has_[i1_Index]));
  ret.Incorporate(d->Find(s1_FieldHash, &s1, s1_Default, has_[s1_Index]));
  return ret;
}

void Randomize(SubSubM* c, std::mt19937_64* rng) {
  std::uniform_int_distribution<int> dist_bool(0, 1);
  std::uniform_int_distribution<int32_t> dist_i32(
      std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
  std::uniform_int_distribution<size_t> dist_len(0, 33);
  std::uniform_int_distribution<int> dist_char(35, 126);

  c->b1 = dist_bool(*rng) != 0;
  c->i1 = dist_i32(*rng);

  size_t len = dist_len(*rng);
  c->s1.resize(len);
  for (size_t i = 0; i < len; ++i) {
    c->s1[i] = static_cast<char>(dist_char(*rng));
  }
};

struct SubM;
struct SubV;

struct SubBase {
  using MaterializedType = SubM;
  using ViewType = SubV;

  // Note -> real ones have type hashes now.
  static constexpr uint32_t kFieldCount = 3;
  static constexpr core::CRC32C u64a_FieldHash = core::CRC32C(21);
  static constexpr core::CRC32C sub1_FieldHash = core::CRC32C(22);
  static constexpr core::CRC32C vsub1_FieldHash = core::CRC32C(23);
  static constexpr uint64_t u64a_Default = 9;
  static constexpr int u64a_Index = 0;
  static constexpr int sub1_Index = 1;
  static constexpr int vsub1_Index = 2;

  bool has_u64a() const { return has() && has_[u64a_Index]; }
  bool has_sub1() const { return has() && has_[sub1_Index]; }
  bool has_vsub1() const { return has() && has_[vsub1_Index]; }
  bool has() const { return has_.size() == kFieldCount; }

  std::vector<bool> has_;
};

struct SubM final : public SubBase {
  uint64_t u64a;
  SubSubM sub1;
  std::vector<SubSubM> vsub1;

  size_t ComputeStorageSize() const;
  void Encode(::capsule::Encoder* e) const;
  Result Decode(::capsule::Decoder* d);
  std::string ToString(int indent = 0) const;
  Result ParseFrom(::capsule::text::TextParser* p);
};

Result SubM::ParseFrom(::capsule::text::TextParser* p) {
  ParsingWidget widget;
  widget.Add("u64a", &u64a);
  widget.AddCapsule("sub1", [this](auto* p) { return sub1.ParseFrom(p); });
  widget.AddCapsuleVector(
      "vsub1", [this](auto* p) { return vsub1.emplace_back().ParseFrom(p); });
  return widget.ParseFrom(p);
}

std::string SubM::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_u64a())
    oss << Spaces(indent) << "u64a" << ": " << u64a << std::endl;
  if (!has() || has_sub1()) {
    oss << Spaces(indent) << "sub1" << " {" << std::endl;
    oss << sub1.ToString(indent + 2);
    oss << Spaces(indent) << "}" << std::endl;
  }
  if ((!has() || has_vsub1()) && !vsub1.empty()) {
    oss << Spaces(indent) << "vsub1" << "[]";
    for (auto& elem : vsub1) {
      oss << " {" << std::endl;
      oss << elem.ToString(indent + 2);
      oss << Spaces(indent) << "}";
    }
    oss << std::endl;
  }
  return oss.str();
}

size_t SubM::ComputeStorageSize() const {
  ::capsule::SizeBuilder sb;
  sb.Add(u64a);
  sb.Add(sub1);
  sb.Add(vsub1);
  return sb.Build();
}

void SubM::Encode(::capsule::Encoder* e) const {
  e->Add(u64a_FieldHash, u64a);
  e->Add(sub1_FieldHash, sub1);
  e->AddCapsuleVector(vsub1_FieldHash, vsub1);
}

Result SubM::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(
      d->Find(u64a_FieldHash, &u64a, u64a_Default, has_[u64a_Index]));
  ret.Incorporate(d->FindCapsule(sub1_FieldHash, &sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->FindCapsuleVector(vsub1_FieldHash, &vsub1, has_[vsub1_Index]));
  return ret;
}

void Randomize(SubM* c, std::mt19937_64* rng) {
  std::uniform_int_distribution<uint64_t> dist_u64(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  std::uniform_int_distribution<size_t> dist_vec_len(0, 4);

  c->u64a = dist_u64(*rng);
  Randomize(&c->sub1, rng);

  size_t len = dist_vec_len(*rng);
  c->vsub1.resize(len);
  for (auto& item : c->vsub1) {
    Randomize(&item, rng);
  }
}

struct TopLevelM;
struct TopLevelV;

struct TopLevelBase {
  using MaterializedType = TopLevelM;
  using ViewType = TopLevelV;

  static constexpr uint32_t kFieldCount = 13;
  static constexpr core::CRC32C u64a_FieldHash = core::CRC32C(101);
  static constexpr core::CRC32C i64a_FieldHash = core::CRC32C(102);
  static constexpr core::CRC32C u32a_FieldHash = core::CRC32C(103);
  static constexpr core::CRC32C i32a_FieldHash = core::CRC32C(104);
  static constexpr core::CRC32C u16a_FieldHash = core::CRC32C(105);
  static constexpr core::CRC32C i16a_FieldHash = core::CRC32C(106);
  static constexpr core::CRC32C u8a_FieldHash = core::CRC32C(107);
  static constexpr core::CRC32C i8a_FieldHash = core::CRC32C(108);
  static constexpr core::CRC32C b1_FieldHash = core::CRC32C(109);
  static constexpr core::CRC32C vs1_FieldHash = core::CRC32C(110);
  static constexpr core::CRC32C sub1_FieldHash = core::CRC32C(111);
  static constexpr core::CRC32C f32a_FieldHash = core::CRC32C(112);
  static constexpr core::CRC32C f64a_FieldHash = core::CRC32C(113);
  static constexpr uint64_t u64a_Default = 99;
  static constexpr int64_t i64a_Default = -999;
  static constexpr uint32_t u32a_Default = 88;
  static constexpr int32_t i32a_Default = -888;
  static constexpr uint16_t u16a_Default = 77;
  static constexpr int16_t i16a_Default = -777;
  static constexpr uint8_t u8a_Default = 6;
  static constexpr int8_t i8a_Default = -6;
  static constexpr bool b1_Default = true;
  static constexpr float f32a_Default = 123.4;
  static constexpr double f64a_Default = 55123.4;
  static constexpr int u64a_Index = 0;
  static constexpr int i64a_Index = 1;
  static constexpr int u32a_Index = 2;
  static constexpr int i32a_Index = 3;
  static constexpr int u16a_Index = 4;
  static constexpr int i16a_Index = 5;
  static constexpr int u8a_Index = 6;
  static constexpr int i8a_Index = 7;
  static constexpr int b1_Index = 8;
  static constexpr int vs1_Index = 9;
  static constexpr int sub1_Index = 10;
  static constexpr int f32a_Index = 11;
  static constexpr int f64a_Index = 12;

  bool has_u64a() const { return has() && has_[u64a_Index]; }
  bool has_i64a() const { return has() && has_[i64a_Index]; }
  bool has_u32a() const { return has() && has_[u32a_Index]; }
  bool has_i32a() const { return has() && has_[i32a_Index]; }
  bool has_u16a() const { return has() && has_[u16a_Index]; }
  bool has_i16a() const { return has() && has_[i16a_Index]; }
  bool has_u8a() const { return has() && has_[u8a_Index]; }
  bool has_i8a() const { return has() && has_[i8a_Index]; }
  bool has_b1() const { return has() && has_[b1_Index]; }
  bool has_vs1() const { return has() && has_[vs1_Index]; }
  bool has_sub1() const { return has() && has_[sub1_Index]; }
  bool has_f32a() const { return has() && has_[f32a_Index]; }
  bool has_f64a() const { return has() && has_[f64a_Index]; }
  bool has() const { return has_.size() == kFieldCount; }

  std::vector<bool> has_;
};

struct TopLevelM final : public TopLevelBase {
  uint64_t u64a;
  int64_t i64a;
  uint32_t u32a;
  int32_t i32a;
  uint16_t u16a;
  int16_t i16a;
  uint8_t u8a;
  int8_t i8a;
  bool b1;
  std::vector<std::string> vs1;
  SubM sub1;
  float f32a;
  double f64a;

  size_t ComputeStorageSize() const;
  void Encode(::capsule::Encoder* e) const;
  Result Decode(::capsule::Decoder* d);
  std::string ToString(int indent = 0) const;
  Result ParseFrom(::capsule::text::TextParser* p);
};

Result TopLevelM::ParseFrom(::capsule::text::TextParser* p) {
  return Result::Ok();
}

std::string TopLevelM::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_u64a())
    oss << Spaces(indent) << "u64a" << ": " << u64a << std::endl;
  if (!has() || has_i64a())
    oss << Spaces(indent) << "i64a" << ": " << i64a << std::endl;
  if (!has() || has_u32a())
    oss << Spaces(indent) << "u32a" << ": " << u32a << std::endl;
  if (!has() || has_i32a())
    oss << Spaces(indent) << "i32a" << ": " << i32a << std::endl;
  if (!has() || has_u16a())
    oss << Spaces(indent) << "u16a" << ": " << u16a << std::endl;
  if (!has() || has_i16a())
    oss << Spaces(indent) << "i16a" << ": " << i16a << std::endl;
  if (!has() || has_u8a())
    oss << Spaces(indent) << "u8a" << ": " << static_cast<uint16_t>(u8a)
        << std::endl;
  if (!has() || has_i8a())
    oss << Spaces(indent) << "i8a" << ": " << static_cast<int16_t>(i8a)
        << std::endl;
  if (!has() || has_b1())
    oss << Spaces(indent) << "b1" << ": " << b1 << std::endl;
  if ((!has() || has_vs1()) && !vs1.empty()) {
    oss << Spaces(indent) << "vs1" << " {" << std::endl;
    for (const auto& s : vs1) {
      oss << Spaces(indent + 2) << "\"" << strings::EscapeString(s) << "\","
          << std::endl;
    }
    oss << Spaces(indent) << "}" << std::endl;
  }
  if (!has() || has_sub1()) {
    oss << Spaces(indent) << "sub1" << " {" << std::endl;
    oss << sub1.ToString(indent + 2);
    oss << Spaces(indent) << "}" << std::endl;
  }
  if (!has() || has_f32a())
    oss << Spaces(indent) << "f32a" << ": " << f32a << std::endl;
  if (!has() || has_f64a())
    oss << Spaces(indent) << "f64a" << ": " << f64a << std::endl;
  return oss.str();
}

size_t TopLevelM::ComputeStorageSize() const {
  ::capsule::SizeBuilder sb;
  sb.Add(u64a);
  sb.Add(i64a);
  sb.Add(u32a);
  sb.Add(i32a);
  sb.Add(u16a);
  sb.Add(i16a);
  sb.Add(u8a);
  sb.Add(i8a);
  sb.Add(b1);
  sb.Add(vs1);
  sb.Add(sub1);
  sb.Add(f32a);
  sb.Add(f64a);
  return sb.Build();
}

void TopLevelM::Encode(::capsule::Encoder* e) const {
  e->Add(u64a_FieldHash, u64a);
  e->Add(i64a_FieldHash, i64a);
  e->Add(u32a_FieldHash, u32a);
  e->Add(i32a_FieldHash, i32a);
  e->Add(u16a_FieldHash, u16a);
  e->Add(i16a_FieldHash, i16a);
  e->Add(u8a_FieldHash, u8a);
  e->Add(i8a_FieldHash, i8a);
  e->Add(b1_FieldHash, b1);
  e->Add(vs1_FieldHash, vs1);
  e->Add(sub1_FieldHash, sub1);
  e->Add(f32a_FieldHash, f32a);
  e->Add(f64a_FieldHash, f64a);
}

Result TopLevelM::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(
      d->Find(u64a_FieldHash, &u64a, u64a_Default, has_[u64a_Index]));
  ret.Incorporate(
      d->Find(i64a_FieldHash, &i64a, i64a_Default, has_[i64a_Index]));
  ret.Incorporate(
      d->Find(u32a_FieldHash, &u32a, u32a_Default, has_[u32a_Index]));
  ret.Incorporate(
      d->Find(i32a_FieldHash, &i32a, i32a_Default, has_[i32a_Index]));
  ret.Incorporate(
      d->Find(u16a_FieldHash, &u16a, u16a_Default, has_[u16a_Index]));
  ret.Incorporate(
      d->Find(i16a_FieldHash, &i16a, i16a_Default, has_[i16a_Index]));
  ret.Incorporate(d->Find(u8a_FieldHash, &u8a, u8a_Default, has_[u8a_Index]));
  ret.Incorporate(d->Find(i8a_FieldHash, &i8a, i8a_Default, has_[i8a_Index]));
  ret.Incorporate(d->Find(b1_FieldHash, &b1, b1_Default, has_[b1_Index]));
  ret.Incorporate(d->FindStringVector(vs1_FieldHash, &vs1, has_[vs1_Index]));
  ret.Incorporate(d->FindCapsule(sub1_FieldHash, &sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->Find(f32a_FieldHash, &f32a, f32a_Default, has_[f32a_Index]));
  ret.Incorporate(
      d->Find(f64a_FieldHash, &f64a, f64a_Default, has_[f64a_Index]));
  return ret;
}

void Randomize(TopLevelM* c, std::mt19937_64* rng) {
  std::uniform_int_distribution<uint64_t> dist_u64(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  std::uniform_int_distribution<int64_t> dist_i64(
      std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
  std::uniform_int_distribution<uint32_t> dist_u32(
      std::numeric_limits<uint32_t>::min(),
      std::numeric_limits<uint32_t>::max());
  std::uniform_int_distribution<int32_t> dist_i32(
      std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
  std::uniform_int_distribution<uint16_t> dist_u16(
      std::numeric_limits<uint16_t>::min(),
      std::numeric_limits<uint16_t>::max());
  std::uniform_int_distribution<int16_t> dist_i16(
      std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max());
  std::uniform_int_distribution<unsigned int> dist_u8(
      std::numeric_limits<uint8_t>::min(), std::numeric_limits<uint8_t>::max());
  std::uniform_int_distribution<int> dist_i8(
      std::numeric_limits<int8_t>::min(), std::numeric_limits<int8_t>::max());
  std::uniform_int_distribution<int> dist_bool(0, 1);
  std::uniform_int_distribution<size_t> dist_vec_len(0, 4);
  std::uniform_int_distribution<size_t> dist_str_len(0, 33);
  std::uniform_int_distribution<int> dist_char(35, 126);
  std::uniform_real_distribution<float> dist_f32(
      std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
  std::uniform_real_distribution<double> dist_f64(
      std::numeric_limits<double>::lowest(),
      std::numeric_limits<double>::max());

  c->u64a = dist_u64(*rng);
  c->i64a = dist_i64(*rng);
  c->u32a = dist_u32(*rng);
  c->i32a = dist_i32(*rng);
  c->u16a = dist_u16(*rng);
  c->i16a = dist_i16(*rng);
  c->u8a = static_cast<uint8_t>(dist_u8(*rng));
  c->i8a = static_cast<int8_t>(dist_i8(*rng));
  c->b1 = dist_bool(*rng) != 0;

  size_t vs1_len = dist_vec_len(*rng);
  c->vs1.resize(vs1_len);
  for (auto& s : c->vs1) {
    size_t slen = dist_str_len(*rng);
    s.resize(slen);
    for (size_t i = 0; i < slen; ++i) {
      s[i] = static_cast<char>(dist_char(*rng));
    }
  }

  Randomize(&c->sub1, rng);

  c->f32a = dist_f32(*rng);
  c->f64a = dist_f64(*rng);
}

struct SubSubV final : public SubSubBase {
  bool b1;
  int32_t i1;
  std::string_view s1;

  std::unique_ptr<::capsule::Storage> ref_;

  Result Decode(::capsule::Decoder* d);
  void RefIfNeeded(::capsule::Storage* s);
  std::string ToString(int indent = 0) const;
};

std::string SubSubV::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_b1())
    oss << Spaces(indent) << "b1" << ": " << b1 << std::endl;
  if (!has() || has_i1())
    oss << Spaces(indent) << "i1" << ": " << i1 << std::endl;
  if (!has() || has_s1())
    oss << Spaces(indent) << "s1" << ": \"" << s1 << "\"" << std::endl;
  return oss.str();
}

Result SubSubV::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(
      d->Find<decltype(b1)>(b1_FieldHash, &b1, b1_Default, has_[b1_Index]));
  ret.Incorporate(
      d->Find<decltype(i1)>(i1_FieldHash, &i1, i1_Default, has_[i1_Index]));
  ret.Incorporate(
      d->Find<decltype(s1)>(s1_FieldHash, &s1, s1_Default, has_[s1_Index]));
  return ret;
}

void SubSubV::RefIfNeeded(::capsule::Storage* s) { ref_ = s->Ref(); }

void Compare(const SubSubM* l, const SubSubM* r) {
  ASSERT_EQ(SubSubM::kFieldCount, r->has_.size());
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_TRUE(r->has_b1());
  EXPECT_EQ(l->i1, r->i1);
  EXPECT_TRUE(r->has_i1());
  EXPECT_EQ(l->s1, r->s1);
  EXPECT_TRUE(r->has_s1());
}

void Compare(const SubSubM* l, const SubSubV* r) {
  ASSERT_EQ(SubSubV::kFieldCount, r->has_.size());
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_TRUE(r->has_b1());
  EXPECT_EQ(l->i1, r->i1);
  EXPECT_TRUE(r->has_i1());
  EXPECT_EQ(l->s1, r->s1);
  EXPECT_TRUE(r->has_s1());
}

void Compare(const SubM* l, const SubM* r) {
  EXPECT_EQ(l->u64a, r->u64a);
  EXPECT_TRUE(r->has_u64a());
  Compare(&l->sub1, &r->sub1);
  EXPECT_TRUE(r->has_vsub1());
  ASSERT_EQ(l->vsub1.size(), r->vsub1.size());
  for (int i = 0; i < l->vsub1.size(); ++i) {
    Compare(&l->vsub1[i], &r->vsub1[i]);
  }
}

void Compare(const TopLevelM* l, const TopLevelM* r) {
  EXPECT_TRUE(r->has_u64a());
  EXPECT_TRUE(r->has_i64a());
  EXPECT_TRUE(r->has_u32a());
  EXPECT_TRUE(r->has_i32a());
  EXPECT_TRUE(r->has_u16a());
  EXPECT_TRUE(r->has_i16a());
  EXPECT_TRUE(r->has_u8a());
  EXPECT_TRUE(r->has_i8a());
  EXPECT_TRUE(r->has_b1());
  EXPECT_TRUE(r->has_vs1());
  EXPECT_TRUE(r->has_sub1());
  EXPECT_TRUE(r->has_f32a());
  EXPECT_TRUE(r->has_f64a());
  EXPECT_EQ(l->u64a, r->u64a);
  EXPECT_EQ(l->i64a, r->i64a);
  EXPECT_EQ(l->u32a, r->u32a);
  EXPECT_EQ(l->i32a, r->i32a);
  EXPECT_EQ(l->u16a, r->u16a);
  EXPECT_EQ(l->i16a, r->i16a);
  EXPECT_EQ(l->u8a, r->u8a);
  EXPECT_EQ(l->i8a, r->i8a);
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_EQ(l->f32a, r->f32a);
  EXPECT_EQ(l->f64a, r->f64a);
  Compare(&l->sub1, &r->sub1);
  ASSERT_EQ(l->vs1.size(), r->vs1.size());
  for (int i = 0; i < l->vs1.size(); ++i) {
    EXPECT_EQ(l->vs1[i], r->vs1[i]);
  }
}

struct SubV final : public SubBase {
  uint64_t u64a;
  SubSubM sub1;
  std::vector<SubSubV> vsub1;

  Result Decode(::capsule::Decoder* d);
  void RefIfNeeded(::capsule::Storage* s);
  std::string ToString(int indent = 0) const;
};

void Compare(const SubM* l, const SubV* r) {
  EXPECT_EQ(l->u64a, r->u64a);
  EXPECT_TRUE(r->has_u64a());
  Compare(&l->sub1, &r->sub1);
  EXPECT_TRUE(r->has_vsub1());
  ASSERT_EQ(l->vsub1.size(), r->vsub1.size());
  for (int i = 0; i < l->vsub1.size(); ++i) {
    Compare(&l->vsub1[i], &r->vsub1[i]);
  }
}

Result SubV::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(d->Find<decltype(u64a)>(u64a_FieldHash, &u64a, u64a_Default,
                                          has_[u64a_Index]));
  ret.Incorporate(d->FindCapsule(sub1_FieldHash, &sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->FindCapsuleVector(vsub1_FieldHash, &vsub1, has_[vsub1_Index]));
  return ret;
}

void SubV::RefIfNeeded(::capsule::Storage* s) {}

std::string SubV::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_u64a())
    oss << Spaces(indent) << "u64a" << ": " << u64a << std::endl;
  if (!has() || has_sub1()) {
    oss << Spaces(indent) << "sub1" << " {" << std::endl;
    oss << sub1.ToString(indent + 2);
    oss << Spaces(indent) << "}" << std::endl;
  }
  if ((!has() || has_vsub1()) && !vsub1.empty()) {
    oss << Spaces(indent) << "vsub1" << "[]";
    for (auto& elem : vsub1) {
      oss << " {" << std::endl;
      oss << elem.ToString(indent + 2);
      oss << Spaces(indent) << "}";
    }
    oss << std::endl;
  }
  return oss.str();
}

struct TopLevelV final : public TopLevelBase {
  uint64_t u64a;
  int64_t i64a;
  uint32_t u32a;
  int32_t i32a;
  uint16_t u16a;
  int16_t i16a;
  uint8_t u8a;
  int8_t i8a;
  bool b1;
  std::vector<std::string_view> vs1;
  SubV sub1;
  float f32a;
  double f64a;

  Result Decode(::capsule::Decoder* d);
  void RefIfNeeded(::capsule::Storage* s);
  std::string ToString(int indent = 0) const;

  std::shared_ptr<::capsule::Storage> ref_;
};

std::string TopLevelV::ToString(int indent) const {
  std::ostringstream oss;
  if (!has() || has_u64a())
    oss << Spaces(indent) << "u64a" << ": " << u64a << std::endl;
  if (!has() || has_i64a())
    oss << Spaces(indent) << "i64a" << ": " << i64a << std::endl;
  if (!has() || has_u32a())
    oss << Spaces(indent) << "u32a" << ": " << u32a << std::endl;
  if (!has() || has_i32a())
    oss << Spaces(indent) << "i32a" << ": " << i32a << std::endl;
  if (!has() || has_u16a())
    oss << Spaces(indent) << "u16a" << ": " << u16a << std::endl;
  if (!has() || has_i16a())
    oss << Spaces(indent) << "i16a" << ": " << i16a << std::endl;
  if (!has() || has_u8a())
    oss << Spaces(indent) << "u8a" << ": " << static_cast<uint16_t>(u8a)
        << std::endl;
  if (!has() || has_i8a())
    oss << Spaces(indent) << "i8a" << ": " << static_cast<int16_t>(i8a)
        << std::endl;
  if (!has() || has_b1())
    oss << Spaces(indent) << "b1" << ": " << b1 << std::endl;
  if ((!has() || has_vs1()) && !vs1.empty()) {
    oss << Spaces(indent) << "vs1" << " {" << std::endl;
    for (const auto& s : vs1) {
      oss << Spaces(indent + 2) << "\"" << s << "\"," << std::endl;
    }
    oss << Spaces(indent) << "}" << std::endl;
  }
  if (!has() || has_sub1()) {
    oss << Spaces(indent) << "sub1" << " {" << std::endl;
    oss << sub1.ToString(indent + 2);
    oss << Spaces(indent) << "}" << std::endl;
  }
  if (!has() || has_f32a())
    oss << Spaces(indent) << "f32a" << ": " << f32a << std::endl;
  if (!has() || has_f64a())
    oss << Spaces(indent) << "f64a" << ": " << f64a << std::endl;
  return oss.str();
}

void Compare(const TopLevelM* l, const TopLevelV* r) {
  EXPECT_TRUE(r->has_u64a());
  EXPECT_TRUE(r->has_i64a());
  EXPECT_TRUE(r->has_u32a());
  EXPECT_TRUE(r->has_i32a());
  EXPECT_TRUE(r->has_u16a());
  EXPECT_TRUE(r->has_i16a());
  EXPECT_TRUE(r->has_u8a());
  EXPECT_TRUE(r->has_i8a());
  EXPECT_TRUE(r->has_b1());
  EXPECT_TRUE(r->has_vs1());
  EXPECT_TRUE(r->has_sub1());
  EXPECT_TRUE(r->has_f32a());
  EXPECT_TRUE(r->has_f64a());
  EXPECT_EQ(l->u64a, r->u64a);
  EXPECT_EQ(l->i64a, r->i64a);
  EXPECT_EQ(l->u32a, r->u32a);
  EXPECT_EQ(l->i32a, r->i32a);
  EXPECT_EQ(l->u16a, r->u16a);
  EXPECT_EQ(l->i16a, r->i16a);
  EXPECT_EQ(l->u8a, r->u8a);
  EXPECT_EQ(l->i8a, r->i8a);
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_EQ(l->f32a, r->f32a);
  EXPECT_EQ(l->f64a, r->f64a);
  Compare(&l->sub1, &r->sub1);
  ASSERT_EQ(l->vs1.size(), r->vs1.size());
  for (int i = 0; i < l->vs1.size(); ++i) {
    EXPECT_EQ(l->vs1[i], r->vs1[i]);
  }
}

Result TopLevelV::Decode(::capsule::Decoder* d) {
  has_.resize(kFieldCount, false);
  Code ret = Code::kOk;
  ret.Incorporate(
      d->Find(u64a_FieldHash, &u64a, u64a_Default, has_[u64a_Index]));
  ret.Incorporate(
      d->Find(i64a_FieldHash, &i64a, i64a_Default, has_[i64a_Index]));
  ret.Incorporate(
      d->Find(u32a_FieldHash, &u32a, u32a_Default, has_[u32a_Index]));
  ret.Incorporate(
      d->Find(i32a_FieldHash, &i32a, i32a_Default, has_[i32a_Index]));
  ret.Incorporate(
      d->Find(u16a_FieldHash, &u16a, u16a_Default, has_[u16a_Index]));
  ret.Incorporate(
      d->Find(i16a_FieldHash, &i16a, i16a_Default, has_[i16a_Index]));
  ret.Incorporate(d->Find(u8a_FieldHash, &u8a, u8a_Default, has_[u8a_Index]));
  ret.Incorporate(d->Find(i8a_FieldHash, &i8a, i8a_Default, has_[i8a_Index]));
  ret.Incorporate(d->Find(b1_FieldHash, &b1, b1_Default, has_[b1_Index]));
  ret.Incorporate(d->FindStringVector(vs1_FieldHash, &vs1, has_[vs1_Index]));
  ret.Incorporate(d->FindCapsule(sub1_FieldHash, &sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->Find(f32a_FieldHash, &f32a, f32a_Default, has_[f32a_Index]));
  ret.Incorporate(
      d->Find(f64a_FieldHash, &f64a, f64a_Default, has_[f64a_Index]));
  return ret;
}

void TopLevelV::RefIfNeeded(::capsule::Storage* s) { ref_ = s->Ref(); }

template <typename CAPSULE>
void RunTranscodeTest(std::unique_ptr<CAPSULE> m) {
  // Compute the necessary storage size.
  const auto capsule_storage_size = m->ComputeStorageSize();
  Log(INFO) << "Capsule reports own size as: " << capsule_storage_size;
  ASSERT_EQ(0, capsule_storage_size % 8);

  // Allocate the necessary storage size.
  auto fac = capsule::NewHeapStorageFactory().ValueOrDie();
  auto storage =
      capsule::Storage::Allocate(fac, capsule_storage_size).ValueOrDie();
  ASSERT_EQ(capsule_storage_size, storage->n());
  auto* const base = storage->template DataAsPtrTo<void>();
  ASSERT_EQ(reinterpret_cast<uintptr_t>(base) % 8, 0);

  // Encode.
  capsule::Encoder e(base, capsule_storage_size, CAPSULE::kFieldCount);
  m->Encode(&e);
  ASSERT_THAT(e.result(), IsOk()) << e.result();
  ASSERT_EQ(e.Seal(), capsule_storage_size);
  Log(INFO) << "Capsule encoded and sealed.";

  // Decode.
  auto d = capsule::Decoder::Build(base, capsule_storage_size).ValueOrDie();
  Log(INFO) << "Decoder built.";

  auto m2 = std::make_unique<CAPSULE>();
  EXPECT_THAT(m2->Decode(&d), IsOk());

  Compare(m.get(), m2.get());

  // Build a view instead of a materialized.
  auto v = std::make_unique<typename CAPSULE::ViewType>();
  v->RefIfNeeded(storage.get());
  EXPECT_THAT(v->Decode(&d), IsOk());
  Compare(m.get(), v.get());
  EXPECT_EQ(v->ToString(), m2->ToString());

  // Now parse m3 from v's ToString().
  std::string text = v->ToString();
  Log(INFO) << text;
  capsule::text::TextParser p(text);
  auto m3 = std::make_unique<CAPSULE>();
  EXPECT_THAT(m3->ParseFrom(&p), IsOk());
  Compare(m3.get(), v.get());
}

template <typename CAPSULE>
void Randomize(CAPSULE* c, bool use_random_seed = false) {
  int seed = 4;
  if (use_random_seed) {
    seed = CycleTime::Now().value() & 0xFFFF;
  }
  std::mt19937_64 gen(seed);
  Log(INFO) << "Seed is " << seed;
  Randomize(c, &gen);
}

TEST(SubSubMTest) {
  auto m = std::make_unique<SubSubM>();
  Randomize(m.get());
  RunTranscodeTest(std::move(m));
}

TEST(SubMTest) {
  auto m = std::make_unique<SubM>();
  Randomize(m.get());
  RunTranscodeTest(std::move(m));
}

TEST(DISABLED_TopLevelMTest) {
  auto m = std::make_unique<TopLevelM>();
  Randomize(m.get());
  RunTranscodeTest(std::move(m));
}

TEST(DISABLED_SubSubMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<SubSubM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

TEST(DISABLED_SubMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<SubM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

TEST(DISABLED_TopLevelMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<TopLevelM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

}  // namespace

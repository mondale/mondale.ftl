#include <random>
#include <vector>
// TODO - need randomness support in runtime

#include "base/logging.h"
#include "capsule/codec.h"
#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/size_builder.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "testing/testing.h"

using testing::IsOk;

namespace {

struct MaterializedInterface {
 public:
  virtual size_t ComputeStorageSize() const = 0;
  virtual void Encode(::capsule::Encoder* e) const = 0;
  virtual Result Decode(::capsule::Decoder* d) = 0;
};

struct ViewInterface {
 public:
  virtual Result Decode(::capsule::Decoder* d) = 0;
  virtual void RefIfNeeded(std::shared_ptr<::capsule::Storage> s) = 0;
};

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

  bool has_b1() const { return has_[b1_Index]; }
  bool has_i1() const { return has_[i1_Index]; }
  bool has_s1() const { return has_[s1_Index]; }

  std::vector<bool> has_;
};

struct SubSubM final : public SubSubBase, public MaterializedInterface {
  bool b1;
  int32_t i1;
  std::string s1;

  size_t ComputeStorageSize() const override;
  void Encode(::capsule::Encoder* e) const override;
  Result Decode(::capsule::Decoder* d) override;

  // not part of generated code...
  void Randomize(std::mt19937_64* rng);
};

void Compare(const SubSubM* l, const SubSubM* r) {
  ASSERT_EQ(SubSubM::kFieldCount, r->has_.size());
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_TRUE(r->has_b1());
  EXPECT_EQ(l->i1, r->i1);
  EXPECT_TRUE(r->has_i1());
  EXPECT_EQ(l->s1, r->s1);
  EXPECT_TRUE(r->has_s1());
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

void SubSubM::Randomize(std::mt19937_64* rng) {
  std::uniform_int_distribution<int> dist_bool(0, 1);
  std::uniform_int_distribution<int32_t> dist_i32(
      std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
  std::uniform_int_distribution<size_t> dist_len(0, 33);
  std::uniform_int_distribution<int> dist_char(32, 126);

  b1 = dist_bool(*rng) != 0;
  i1 = dist_i32(*rng);

  size_t len = dist_len(*rng);
  s1.resize(len);
  for (size_t i = 0; i < len; ++i) {
    s1[i] = static_cast<char>(dist_char(*rng));
  }
};

struct SubM;
struct SubV;

struct SubBase {
  using MaterializedType = SubM;
  using ViewType = SubV;

  static constexpr uint32_t kFieldCount = 3;
  static constexpr core::CRC32C u64a_FieldHash = core::CRC32C(21);
  static constexpr core::CRC32C sub1_FieldHash = core::CRC32C(22);
  static constexpr core::CRC32C vsub1_FieldHash = core::CRC32C(23);
  static constexpr uint64_t u64a_Default = 9;
  static constexpr int u64a_Index = 0;
  static constexpr int sub1_Index = 1;
  static constexpr int vsub1_Index = 2;

  bool has_u64a() const { return has_[u64a_Index]; }
  bool has_sub1() const { return has_[sub1_Index]; }
  bool has_vsub1() const { return has_[vsub1_Index]; }

  std::vector<bool> has_;
};

struct SubM final : public SubBase, public MaterializedInterface {
  uint64_t u64a;
  SubSubM sub1;
  std::vector<SubSubM> vsub1;

  size_t ComputeStorageSize() const override;
  void Encode(::capsule::Encoder* e) const override;
  Result Decode(::capsule::Decoder* d) override;

  // Not part of generated code.
  void Randomize(std::mt19937_64* rng);
};

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
  ret.Incorporate(
      d->FindCapsule(sub1_FieldHash, &sub1, sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->FindCapsuleVector(vsub1_FieldHash, &vsub1, has_[vsub1_Index]));
  return ret;
}

void SubM::Randomize(std::mt19937_64* rng) {
  std::uniform_int_distribution<uint64_t> dist_u64(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  std::uniform_int_distribution<size_t> dist_vec_len(0, 4);

  u64a = dist_u64(*rng);
  sub1.Randomize(rng);

  size_t len = dist_vec_len(*rng);
  vsub1.resize(len);
  for (auto& item : vsub1) {
    item.Randomize(rng);
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

  bool has_u64a() const { return has_[u64a_Index]; }
  bool has_i64a() const { return has_[i64a_Index]; }
  bool has_u32a() const { return has_[u32a_Index]; }
  bool has_i32a() const { return has_[i32a_Index]; }
  bool has_u16a() const { return has_[u16a_Index]; }
  bool has_i16a() const { return has_[i16a_Index]; }
  bool has_u8a() const { return has_[u8a_Index]; }
  bool has_i8a() const { return has_[i8a_Index]; }
  bool has_b1() const { return has_[b1_Index]; }
  bool has_vs1() const { return has_[vs1_Index]; }
  bool has_sub1() const { return has_[sub1_Index]; }
  bool has_f32a() const { return has_[f32a_Index]; }
  bool has_f64a() const { return has_[f64a_Index]; }

  std::vector<bool> has_;
};

struct TopLevelM final : public TopLevelBase, public MaterializedInterface {
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

  size_t ComputeStorageSize() const override;
  void Encode(::capsule::Encoder* e) const override;
  Result Decode(::capsule::Decoder* d) override;

  // Not part of generated code.
  void Randomize(std::mt19937_64* rng);
};

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
  ret.Incorporate(
      d->FindCapsule(sub1_FieldHash, &sub1, sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->Find(f32a_FieldHash, &f32a, f32a_Default, has_[f32a_Index]));
  ret.Incorporate(
      d->Find(f64a_FieldHash, &f64a, f64a_Default, has_[f64a_Index]));
  return ret;
}

void TopLevelM::Randomize(std::mt19937_64* rng) {
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
  std::uniform_int_distribution<int> dist_char(32, 126);
  std::uniform_real_distribution<float> dist_f32(
      std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
  std::uniform_real_distribution<double> dist_f64(
      std::numeric_limits<double>::lowest(),
      std::numeric_limits<double>::max());

  u64a = dist_u64(*rng);
  i64a = dist_i64(*rng);
  u32a = dist_u32(*rng);
  i32a = dist_i32(*rng);
  u16a = dist_u16(*rng);
  i16a = dist_i16(*rng);
  u8a = static_cast<uint8_t>(dist_u8(*rng));
  i8a = static_cast<int8_t>(dist_i8(*rng));
  b1 = dist_bool(*rng) != 0;

  size_t vs1_len = dist_vec_len(*rng);
  vs1.resize(vs1_len);
  for (auto& s : vs1) {
    size_t slen = dist_str_len(*rng);
    s.resize(slen);
    for (size_t i = 0; i < slen; ++i) {
      s[i] = static_cast<char>(dist_char(*rng));
    }
  }

  sub1.Randomize(rng);

  f32a = dist_f32(*rng);
  f64a = dist_f64(*rng);
}

struct SubSubV final : public SubSubBase, public ViewInterface {
  bool b1;
  int32_t i1;
  std::string_view s1;

  std::shared_ptr<::capsule::Storage> ref_;

  Result Decode(::capsule::Decoder* d) override;
  void RefIfNeeded(std::shared_ptr<::capsule::Storage> s) override;
};

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

void SubSubV::RefIfNeeded(std::shared_ptr<::capsule::Storage> s) { ref_ = s; }

void Compare(const SubSubM* l, const SubSubV* r) {
  ASSERT_EQ(SubSubV::kFieldCount, r->has_.size());
  EXPECT_EQ(l->b1, r->b1);
  EXPECT_TRUE(r->has_b1());
  EXPECT_EQ(l->i1, r->i1);
  EXPECT_TRUE(r->has_i1());
  EXPECT_EQ(l->s1, r->s1);
  EXPECT_TRUE(r->has_s1());
}

struct SubV final : public SubBase, public ViewInterface {
  uint64_t u64a;
  SubSubM sub1;
  std::vector<SubSubV> vsub1;

  Result Decode(::capsule::Decoder* d) override;
  void RefIfNeeded(std::shared_ptr<::capsule::Storage> s) override;
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
  ret.Incorporate(
      d->FindCapsule(sub1_FieldHash, &sub1, sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->FindCapsuleVector(vsub1_FieldHash, &vsub1, has_[vsub1_Index]));
  return ret;
}

void SubV::RefIfNeeded(std::shared_ptr<::capsule::Storage> s) {}

struct TopLevelV final : public TopLevelBase, public ViewInterface {
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

  Result Decode(::capsule::Decoder* d) override;
  void RefIfNeeded(std::shared_ptr<::capsule::Storage> s) override;

  std::shared_ptr<::capsule::Storage> ref_;
};

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
  ret.Incorporate(
      d->FindCapsule(sub1_FieldHash, &sub1, sub1, has_[sub1_Index]));
  ret.Incorporate(
      d->Find(f32a_FieldHash, &f32a, f32a_Default, has_[f32a_Index]));
  ret.Incorporate(
      d->Find(f64a_FieldHash, &f64a, f64a_Default, has_[f64a_Index]));
  return ret;
}

void TopLevelV::RefIfNeeded(std::shared_ptr<::capsule::Storage> s) { ref_ = s; }

template <typename CAPSULE>
void RunTranscodeTest(std::unique_ptr<CAPSULE> m) {
  // Compute the necessary storage size.
  const auto capsule_storage_size = m->ComputeStorageSize();
  Log(INFO) << "Capsule reports own size as: " << capsule_storage_size;
  ASSERT_EQ(0, capsule_storage_size % 8);

  // Allocate the necessary storage size.
  auto fac = capsule::NewHeapStorageFactory().ValueOrDie();
  auto storage =
      capsule::Storage::Allocate(fac.get(), capsule_storage_size).ValueOrDie();
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
  v->RefIfNeeded(storage);
  EXPECT_THAT(v->Decode(&d), IsOk());
  Compare(m.get(), v.get());
}

template <typename CAPSULE>
void Randomize(CAPSULE* c, bool use_random_seed = false) {
  int seed = 4;
  if (use_random_seed) {
    seed = CycleTime::Now().value() & 0xFFFF;
  }
  std::mt19937_64 gen(seed);
  Log(INFO) << "Seed is " << seed;
  c->Randomize(&gen);
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

TEST(TopLevelMTest) {
  auto m = std::make_unique<TopLevelM>();
  Randomize(m.get());
  RunTranscodeTest(std::move(m));
}

TEST(SubSubMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<SubSubM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

TEST(SubMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<SubM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

TEST(TopLevelMTest100) {
  for (int i = 0; i < 100; ++i) {
    auto m = std::make_unique<TopLevelM>();
    Randomize(m.get(), true);
    RunTranscodeTest(std::move(m));
  }
}

}  // namespace

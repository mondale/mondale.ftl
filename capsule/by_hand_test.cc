#include <random>
// TODO - need randomness support in runtime

#include "base/logging.h"
#include "capsule/abi.h"
#include "capsule/codec.h"
#include "capsule/decoder.h"
#include "capsule/encoder.h"
#include "capsule/size_builder.h"
#include "capsule/storage.h"
#include "capsule/storage_factory.h"
#include "testing/testing.h"

namespace {

struct MaterializedInterface {
 public:
  virtual size_t ComputeStorageSize() const = 0;
  virtual void Encode(::capsule::Encoder* e) const = 0;
};

struct SubSubM final : public MaterializedInterface {
  [[maybe_unused]] static constexpr core::CRC32C kTypeHash = core::CRC32C(30);
  [[maybe_unused]] static constexpr uint32_t kFieldCount = 3;
  static constexpr core::CRC32C b1_FieldHash = core::CRC32C(31);
  static constexpr core::CRC32C i1_FieldHash = core::CRC32C(32);
  static constexpr core::CRC32C s1_FieldHash = core::CRC32C(33);

  bool b1;
  int32_t i1;
  std::string s1;

  size_t ComputeStorageSize() const override;
  void Encode(::capsule::Encoder* e) const override;

  void Randomize(std::mt19937_64* rng);
};

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

struct SubM final : public MaterializedInterface {
  [[maybe_unused]] static constexpr core::CRC32C kTypeHash = core::CRC32C(20);
  [[maybe_unused]] static constexpr uint32_t kFieldCount = 2;
  static constexpr core::CRC32C u64a_FieldHash = core::CRC32C(21);
  static constexpr core::CRC32C vsub1_FieldHash = core::CRC32C(22);

  uint64_t u64a;
  std::vector<SubSubM> vsub1;

  size_t ComputeStorageSize() const override;
  void Encode(::capsule::Encoder* e) const override;

  void Randomize(std::mt19937_64* rng);
};

size_t SubM::ComputeStorageSize() const {
  ::capsule::SizeBuilder sb;
  sb.Add(u64a);
  sb.Add(vsub1);
  return sb.Build();
}

void SubM::Encode(::capsule::Encoder* e) const {
  e->Add(u64a_FieldHash, u64a);
  e->AddCapsuleVector(vsub1_FieldHash, vsub1);
}

void SubM::Randomize(std::mt19937_64* rng) {
  std::uniform_int_distribution<uint64_t> dist_u64(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  std::uniform_int_distribution<size_t> dist_vec_len(0, 4);

  u64a = dist_u64(*rng);

  size_t len = dist_vec_len(*rng);
  vsub1.resize(len);
  for (auto& item : vsub1) {
    item.Randomize(rng);
  }
}

struct TopLevelM final : public MaterializedInterface {
  [[maybe_unused]] static constexpr core::CRC32C kTypeHash = core::CRC32C(100);
  [[maybe_unused]] static constexpr uint32_t kFieldCount = 13;
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

  void Randomize(std::mt19937_64* rng);
};

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

struct SubSubV final {
  bool b1;
  int32_t i1;
  std::string_view s1;
};

struct SubV final {
  uint64_t u64a;
  std::vector<SubSubV> vsub1;
};

struct TopLevelV final {
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
};

constexpr bool kUseRandomSeed = false;
TEST(MsvTest) {
  // M->S->V:
  // Materialized -> serialize to Storage
  // Storage -> parse to View

  // Begin with a random Materialized.
  auto m = std::make_unique<TopLevelM>();
  int seed = 4;
  if (kUseRandomSeed) {
    seed = CycleTime::Now().value() & 0xFFFF;
  }
  std::mt19937_64 gen(seed);
  Log(INFO) << "Seed is " << seed;
  m->Randomize(&gen);

  // Compute the necessary storage size.
  const auto capsule_storage_size = m->ComputeStorageSize();
  Log(INFO) << "Top computes size as: " << capsule_storage_size;
  ASSERT_EQ(0, capsule_storage_size % 8);
  // TODO - consider whether framed capsules should always have len % 8 == 0.
  const auto framed_storage_size = capsule_storage_size +
                                   sizeof(capsule::abi::FrameHeader) +
                                   sizeof(core::CRC32C);
  Log(INFO) << "Framed storage size is: " << framed_storage_size;

  // Allocate the necessary storage size.
  auto fac = capsule::NewHeapStorageFactory().ValueOrDie();
  auto span = fac->NewSpan(framed_storage_size).ValueOrDie();
  ASSERT_EQ(framed_storage_size, span->n());
  ASSERT_EQ(reinterpret_cast<uintptr_t>(span->DataAsPtrTo<void>()) % 8, 0);
}

}  // namespace

#include "capsule/verify.h"
#include "testing/testing.h"

using testing::HasSubstr;
using testing::IsOk;

namespace capsule {

TEST(NoCapsules) {
  CapsuleFile cf;
  cf.capsules.clear();
  EXPECT_THAT(VerifyAtLeastOneCapsule(cf).ToString(),
              HasSubstr("No capsules defined"));
}

TEST(BogusType) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  auto& f = cf.capsules[0].fields[0];
  f.name = "Lolz";
  f.srcloc = "Here";
  f.type = "i9999";
  const auto str = VerifyTypeSoundness(cf).ToString();
  EXPECT_THAT(str, HasSubstr("Here"));
  EXPECT_THAT(str, HasSubstr("i9999"));
  EXPECT_THAT(str, HasSubstr("Unrecognized"));
}

TEST(NameCollisionsWithTypes) {
  CapsuleFile cf;
  cf.capsules.resize(2);
  cf.capsules[0].srcloc = "EvilCapsule";
  cf.capsules[0].name = "u8";
  cf.capsules[1].name = "OkCapsule";
  cf.capsules[0].fields.resize(2);
  cf.capsules[0].fields[0].srcloc = "19";
  cf.capsules[0].fields[0].name = "u32";
  cf.capsules[0].fields[1].srcloc = "20";
  cf.capsules[0].fields[1].name = "OkCapsule";
  const auto str = VerifyNamesDistinctFromTypes(cf).ToString();

  // Capsules can't be named after primitives.
  EXPECT_THAT(str, HasSubstr("EvilCapsule"));
  EXPECT_THAT(str, HasSubstr("u8"));

  // Fields can't be named after primitives.
  EXPECT_THAT(str, HasSubstr("19"));
  EXPECT_THAT(str, HasSubstr("u32"));

  // Fields can't be named after capsules.
  EXPECT_THAT(str, HasSubstr("20"));
  EXPECT_THAT(str, HasSubstr("OkCapsule"));
}

TEST(RepeatedCapsuleNames) {
  CapsuleFile cf;
  cf.capsules.resize(3);
  cf.capsules[0].name = "Jazz";
  cf.capsules[0].srcloc = "001";
  cf.capsules[1].name = "Jazz";
  cf.capsules[1].srcloc = "002";
  cf.capsules[2].name = "Jazz";
  cf.capsules[2].srcloc = "003";
  const auto str = VerifyCapsuleNameUniqueness(cf).ToString();

  EXPECT_THAT(str, HasSubstr("Jazz"));
  EXPECT_THAT(str, HasSubstr("002"));
  EXPECT_THAT(str, HasSubstr("003"));
}

TEST(VerbotenNames) {
  CapsuleFile cf;
  cf.namespace_name = "char";
  EXPECT_THAT(VerifyNamesNotVerboten(cf).ToString(), HasSubstr("char"));

  cf.namespace_name = "okns";
  cf.capsules.resize(1);
  cf.capsules[0].name = "core";
  EXPECT_THAT(VerifyNamesNotVerboten(cf).ToString(), HasSubstr("core"));
  cf.capsules[0].name = "Stuff";

  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].name = "namespace";
  EXPECT_THAT(VerifyNamesNotVerboten(cf).ToString(), HasSubstr("namespace"));
}

TEST(CollisionsWithGeneratedCapsule) {
  // capsule Foo { ... } capsule FooBase { ... }
  CapsuleFile cf;
  cf.capsules.resize(2);
  cf.capsules[0].name = "Foo";
  cf.capsules[1].name = "FooBase";
  EXPECT_THAT(VerifyNoGeneratedNameCollision(cf).ToString(),
              HasSubstr("FooBase"));
}

TEST(CollisionsWithGeneratedCapsuleFieldName) {
  // capsule Foo { u32 FooM }
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].name = "Foo";
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].name = "FooM";
  EXPECT_THAT(VerifyNoGeneratedNameCollision(cf).ToString(), HasSubstr("FooM"));
}

TEST(CollisionsWithGeneratedFieldVariable) {
  // capsule Foo { u32 FooM }
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(2);
  cf.capsules[0].fields[0].name = "blinding";
  cf.capsules[0].fields[1].name = "blinding_FieldHash";
  EXPECT_THAT(VerifyNoGeneratedNameCollision(cf).ToString(),
              HasSubstr("blinding_FieldHash"));
}

TEST(RedundantAttr) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].attributes.resize(2);
  cf.capsules[0].fields[0].attributes[0].name = "retired";
  cf.capsules[0].fields[0].attributes[1].name = "retired";
  EXPECT_THAT(VerifyRecognizedAttributes(cf).ToString(),
              HasSubstr("Attribute [retired] appears redundantly"));
}

TEST(UnrecognizedAttr) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].attributes.resize(1);
  cf.capsules[0].fields[0].attributes[0].name = "smelly";
  EXPECT_THAT(VerifyRecognizedAttributes(cf).ToString(),
              HasSubstr("Unrecognized attribute [smelly]"));
}

TEST(DefaultNoValue) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].attributes.resize(1);
  cf.capsules[0].fields[0].attributes[0].name = "default";
  EXPECT_THAT(VerifyRecognizedAttributes(cf).ToString(),
              HasSubstr("Attribute @default requires a value"));
  cf.capsules[0].fields[0].attributes[0].value = "9";
  EXPECT_THAT(VerifyRecognizedAttributes(cf), IsOk());
}

TEST(FormerlyNoValue) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].attributes.resize(1);
  cf.capsules[0].fields[0].attributes[0].name = "formerly";
  EXPECT_THAT(VerifyRecognizedAttributes(cf).ToString(),
              HasSubstr("Attribute @formerly requires a value"));
  cf.capsules[0].fields[0].attributes[0].value = "foo";
  EXPECT_THAT(VerifyRecognizedAttributes(cf), IsOk());
}

TEST(RetiredAcceptsNoValue) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(1);
  cf.capsules[0].fields[0].attributes.resize(1);
  cf.capsules[0].fields[0].attributes[0].name = "retired";
  cf.capsules[0].fields[0].attributes[0].value = "foo";
  EXPECT_THAT(VerifyRecognizedAttributes(cf).ToString(),
              HasSubstr("Attribute @retired does not take a value"));
  cf.capsules[0].fields[0].attributes[0].value.clear();
  EXPECT_THAT(VerifyRecognizedAttributes(cf), IsOk());
}

TEST(NoDefaultOnVectorOrCapsule) {
  CapsuleFile cf;
  cf.capsules.resize(1);
  cf.capsules[0].fields.resize(2);
  cf.capsules[0].fields[0].type = "vector<u32>";
  cf.capsules[0].fields[0].attributes.resize(1);
  cf.capsules[0].fields[0].attributes[0].name = "default";
  cf.capsules[0].fields[1].type = "Shennanigan";
  cf.capsules[0].fields[1].attributes.resize(1);
  cf.capsules[0].fields[1].attributes[0].name = "default";
  EXPECT_THAT(VerifyNoDefaultsOnVectorsOrCapsules(cf).ToString(),
              HasSubstr("vector<u32>"));
  EXPECT_THAT(VerifyNoDefaultsOnVectorsOrCapsules(cf).ToString(),
              HasSubstr("Shennanigan"));
}

}  // namespace capsule

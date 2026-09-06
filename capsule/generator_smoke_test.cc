#include "capsule/generator.h"
#include "capsule/hashing.h"
#include "capsule/parser.h"
#include "core/file.h"
#include "testing/testing.h"

using ::testing::IsOk;

FLAG_COHORT(generator_smoke_test);
// TODO - bruh you need a boolean flag
// plz run capsule:generator_smoke_test -- --generator_smoke_test.write_golden=1
FLAG(int, write_golden, 0);

namespace {

class GoldenSmokeFixture : public ::testing::Test {
 protected:
  GoldenSmokeFixture() {
    std::string input_schema =
        core::ReadContentsFromFile(
            "capsule/testdata/generator_smoke_test.capsule")
            .ValueOrDie();
    capsule::Parser parser(input_schema, "full_feature.capsule");
    auto parse_result = parser.Parse();
    ASSERT_THAT(parse_result.result(), IsOk());

    cf_ = std::move(parse_result.ValueOrDie());
    ASSERT_THAT(capsule::ComputeAndValidateHashes(&cf_), IsOk());
  }

  capsule::CapsuleFile cf_;
};

TEST_F(GoldenSmokeFixture, GeneratorHeaderTest) {
  auto gen_result = capsule::GenerateHeader(cf_);
  ASSERT_THAT(gen_result.result(), IsOk());
  auto header_contents = std::move(gen_result.ValueOrDie());

  if (FLAG_LOOKUP(write_golden)) {
    CHECK_OK(core::WriteContentsToFile(
        "capsule/testdata/generator_smoke_test.golden.h", header_contents));
  }

  std::string expected = core::ReadContentsFromFile(
                             "capsule/testdata/generator_smoke_test.golden.h")
                             .ValueOrDie();
  EXPECT_EQ(expected, header_contents);
}

TEST_F(GoldenSmokeFixture, GeneratorSourceTest) {
  auto gen_result = capsule::GenerateSource(
      cf_, "capsule/testdata/generator_smoke_test.out.h");
  ASSERT_THAT(gen_result.result(), IsOk());
  auto source_contents = std::move(gen_result.ValueOrDie());

  if (FLAG_LOOKUP(write_golden)) {
    CHECK_OK(core::WriteContentsToFile(
        "capsule/testdata/generator_smoke_test.golden.cc", source_contents));
  }

  std::string expected = core::ReadContentsFromFile(
                             "capsule/testdata/generator_smoke_test.golden.cc")
                             .ValueOrDie();
  EXPECT_EQ(expected, source_contents);
}

}  // namespace

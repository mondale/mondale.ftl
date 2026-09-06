#include "capsule/generator.h"
#include "capsule/hashing.h"
#include "capsule/parser.h"
#include "core/file.h"
#include "testing/testing.h"

using ::testing::IsOk;

namespace {

TEST(GeneratorFullFeatureTest) {
  std::string input_schema =
      core::ReadContentsFromFile(
          "capsule/testdata/generator_smoke_test.capsule")
          .ValueOrDie();
  capsule::Parser parser(input_schema, "full_feature.capsule");
  auto parse_result = parser.Parse();
  ASSERT_THAT(parse_result.result(), IsOk());

  capsule::CapsuleFile cf = std::move(parse_result.ValueOrDie());
  ASSERT_THAT(capsule::ComputeAndValidateHashes(&cf), IsOk());

  auto gen_result = capsule::GenerateHeader(cf);
  ASSERT_THAT(gen_result.result(), IsOk());

  std::string expected = core::ReadContentsFromFile(
                             "capsule/testdata/generator_smoke_test.golden.h")
                             .ValueOrDie();
  EXPECT_EQ(gen_result.ValueOrDie(), expected);
}

}  // namespace

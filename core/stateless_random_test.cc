#include <bit>
#include <vector>

#include "core/stateless_random.h"
#include "testing/testing.h"

namespace {

TEST(StatelessRandomTest_Rand64BasicGeneration) {
  uint64_t val1 = core::Rand64(1);
  uint64_t val2 = core::Rand64(2);

  // Different entropy values should yield different mixed outputs
  EXPECT_NE(val1, val2);
}

TEST(StatelessRandomTest_DefaultArgumentSupport) {
  // Verify that the default entropy parameter (0) compiles and executes
  // correctly
  uint64_t default_val64 = core::Rand64();
  uint32_t default_val32 = core::Rand<uint32_t>();

  // Smoke test assertion
  EXPECT_NE(default_val64,
            default_val32);  // Statistically extremely likely to differ
}

TEST(StatelessRandomTest_SampleRandomnessStatisticalValidity) {
  constexpr int kNumSamples = 1000;
  std::vector<uint32_t> samples;
  samples.reserve(kNumSamples);

  // Collect 32-bit samples using a changing entropy/counter stream
  for (int i = 0; i < kNumSamples; ++i) {
    samples.push_back(core::Rand<uint32_t>(static_cast<uint64_t>(i)));
  }

  // 1. Hamming Weight (Bit Balance) Test
  // 1000 values * 32 bits = 32,000 total bits.
  // Expected set bits = 16,000.
  uint64_t total_set_bits = 0;
  for (uint32_t val : samples) {
    total_set_bits += std::popcount(val);
  }

  constexpr uint64_t kExpectedBits = (kNumSamples * 32) / 2;
  constexpr uint64_t kTolerance = 400;

  EXPECT_GE(total_set_bits, kExpectedBits - kTolerance);
  EXPECT_LE(total_set_bits, kExpectedBits + kTolerance);

  // 2. Mean Value Distribution Test
  // Using uint64_t to accumulate 32-bit values safely without overflow.
  uint64_t sum = 0;
  for (uint32_t val : samples) {
    sum += val;
  }

  double average = static_cast<double>(sum) / kNumSamples;
  double expected_average = static_cast<double>(UINT32_MAX) / 2.0;

  // Expect the sample mean to be within 5% of the theoretical uniform midpoint
  EXPECT_NEAR_ABS(average, expected_average, expected_average * 0.05);
}

TEST(WeightedSelectorTest_EmptyWeightsReturnsError) {
  std::vector<uint32_t> weights;
  ResultOr<size_t> result = core::WeightedSelect(weights);
  EXPECT_FALSE(result.IsOk());
}

TEST(WeightedSelectorTest_ZeroTotalWeightReturnsError) {
  std::vector<uint32_t> weights = {0, 0, 0};
  ResultOr<size_t> result = core::WeightedSelect(weights);
  EXPECT_FALSE(result.IsOk());
}

TEST(WeightedSelectorTest_SingleWeightAlwaysReturnsZero) {
  std::vector<uint32_t> weights = {42};
  ResultOr<size_t> result = core::WeightedSelect(weights);
  ASSERT_TRUE(result.IsOk());
  EXPECT_EQ(result.ValueOrDie(), 0);
}

TEST(WeightedSelectorTest_SelectionFallsWithinValidBounds) {
  std::vector<uint32_t> weights = {10, 20, 30, 40};

  for (int i = 0; i < 100; ++i) {
    ResultOr<size_t> result = core::WeightedSelect(weights);
    ASSERT_TRUE(result.IsOk());
    size_t index = result.ValueOrDie();
    EXPECT_LT(index, weights.size());
  }
}

TEST(RandomUniformTest_EqualBoundsReturnsLow) {
  uint32_t val = core::RandomUniform(42, 42);
  EXPECT_EQ(val, 42);
}

TEST(RandomUniformTest_ResultsWithinBounds) {
  constexpr uint32_t kLow = 10;
  constexpr uint32_t kHigh = 20;

  for (int i = 0; i < 100; ++i) {
    uint32_t val = core::RandomUniform(kLow, kHigh);
    EXPECT_GE(val, kLow);
    EXPECT_LT(val, kHigh);
  }
}

TEST(RandomUniformTest_SampleRandomnessStatisticalValidity) {
  constexpr uint32_t kLow = 0;
  constexpr uint32_t kHigh = 100;
  constexpr int kNumSamples = 1000;

  uint64_t sum = 0;
  for (int i = 0; i < kNumSamples; ++i) {
    uint32_t val = core::RandomUniform(kLow, kHigh);
    EXPECT_GE(val, kLow);
    EXPECT_LT(val, kHigh);
    sum += val;
  }

  double average = static_cast<double>(sum) / kNumSamples;
  // The expected midpoint for a uniform distribution over [0, 100) is 49.5
  double expected_average = static_cast<double>(kLow + kHigh - 1) / 2.0;

  EXPECT_NEAR_ABS(average, expected_average, 3.0);
}

}  // namespace

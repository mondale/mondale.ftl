#include "io/utilization_estimator.h"
#include "testing/testing.h"

namespace {

using io::UtilizationEstimator;

TEST(UtilizationEstimatorTest_InitialEstimateIsZero) {
  UtilizationEstimator estimator;
  EXPECT_EQ(estimator.Estimate(), 0);
}

TEST(UtilizationEstimatorTest_ZeroTimeSampleIgnored) {
  UtilizationEstimator estimator;
  estimator.Sample(0, 0);
  EXPECT_EQ(estimator.Estimate(), 0);
}

TEST(UtilizationEstimatorTest_SingleSampleCalculation) {
  UtilizationEstimator estimator;
  estimator.Sample(50, 50);  // 50% utilization but only recently.
  const int estimate = (0 * 1 + 0 * 2 + 0 * 3 + 4 * 50) / (1 + 2 + 3 + 4);
  EXPECT_EQ(estimator.Estimate(), estimate);
}

TEST(UtilizationEstimatorTest_Steady) {
  UtilizationEstimator estimator;
  for (int i = 0; i < UtilizationEstimator::N; ++i) {
    estimator.Sample(50, 50);  // 50% utilization but only recently.
  }
  EXPECT_EQ(estimator.Estimate(), 50);
}

}  // namespace

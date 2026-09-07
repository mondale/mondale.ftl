#include "capsule/compiler_smoke_test.capsule.h"
#include "testing/testing.h"

namespace {

TEST(DoesItWork) {
  test_ns::FullFeatureM m;  // I guess it works if this compiles.
}

}  // namespace

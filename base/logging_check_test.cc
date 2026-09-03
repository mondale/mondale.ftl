#include "base/logging.h"
#include "testing/testing.h"

using testing::StderrContains;

struct TestResult {
  bool success;
  int code;
};

inline bool IsOk(const TestResult& r) { return r.success; }

inline std::ostream& operator<<(std::ostream& os, const TestResult& r) {
  return os << "Code(" << r.code << ")";
}

TEST(TypeForgiving) {
  CHECK_EQ(0, 0u);
  CHECK_GE(0, 0u);
  CHECK_LE(0, 0u);
  CHECK_GT(1, 0u);
  CHECK_LT(-1, 0u);
  CHECK_NE(-1, 0u);
  DCHECK_EQ(0, 0u);
  DCHECK_GE(0, 0u);
  DCHECK_LE(0, 0u);
  DCHECK_GT(1, 0u);
  DCHECK_LT(-1, 0u);
  DCHECK_NE(-1, 0u);
}

TEST(CheckMacro) {
  EXPECT_DEATH([] { CHECK(false); }, StderrContains("Check failed: false"));
}

TEST(CheckEqMacro) {
  EXPECT_DEATH([] { CHECK_EQ(5, 10); },
               StderrContains("Check failed: 5 == 10"));
}

TEST(CheckNeMacro) {
  EXPECT_DEATH([] { CHECK_NE(5, 5); }, StderrContains("Check failed: 5 != 5"));
}

TEST(CheckLeMacro) {
  EXPECT_DEATH([] { CHECK_LE(10, 5); },
               StderrContains("Check failed: 10 <= 5"));
}

TEST(CheckLtMacro) {
  EXPECT_DEATH([] { CHECK_LT(10, 5); }, StderrContains("Check failed: 10 < 5"));
}

TEST(CheckGeMacro) {
  EXPECT_DEATH([] { CHECK_GE(5, 10); },
               StderrContains("Check failed: 5 >= 10"));
}

TEST(CheckGtMacro) {
  EXPECT_DEATH([] { CHECK_GT(5, 10); }, StderrContains("Check failed: 5 > 10"));
}

TEST(CheckOkMacro) {
  TestResult bad_res{false, 42};
  EXPECT_DEATH([&] { CHECK_OK(bad_res); },
               StderrContains("Check failed: bad_res is not OK: Code\\(42\\)"));
}

TEST(DCheckMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK(false); },
                     StderrContains("Check failed: false"));
}

TEST(DCheckEqMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_EQ(5, 10); },
                     StderrContains("Check failed: 5 == 10"));
}

TEST(DCheckNeMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_NE(5, 5); },
                     StderrContains("Check failed: 5 != 5"));
}

TEST(DCheckLeMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_LE(10, 5); },
                     StderrContains("Check failed: 10 <= 5"));
}

TEST(DCheckLtMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_LT(10, 5); },
                     StderrContains("Check failed: 10 < 5"));
}

TEST(DCheckGeMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_GE(5, 10); },
                     StderrContains("Check failed: 5 >= 10"));
}

TEST(DCheckGtMacro) {
  EXPECT_DEBUG_DEATH([] { DCHECK_GT(5, 10); },
                     StderrContains("Check failed: 5 > 10"));
}

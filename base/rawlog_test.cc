#include <sstream>

#include "base/basic_test.h"
#include "base/rawlog.h"

namespace {

TEST(LogInfoTest) {
  std::stringstream ss;
  ::base::rawlog::TESTONLY_SetInfoStream(&ss);
  RAW_INFO << "Hello " << "Everyone!";
  std::string expected = "I] Hello Everyone!";
  EXPECT_EQ(expected, ss.str());
}

}  // namespace

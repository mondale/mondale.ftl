#include <sstream>

#include "base/basic_test.h"
#include "base/rawlog.h"

namespace {

TEST(LogInfoTest) {
  std::stringstream ss;
  ::base::rawlog::TESTONLY_SetInfoStream(&ss);
  RAW_INFO << "Hello " << "Everyone!";
  std::string expected = "I] Hello Everyone!\n";
  EXPECT_EQ(expected, ss.str());
}

TEST(LogErrorTest) {
  std::stringstream ss;
  ::base::rawlog::TESTONLY_SetErrorStream(&ss);
  RAW_ERROR << "Oh no I had an error :(";
  std::string expected = "E] Oh no I had an error :(\n";
  EXPECT_EQ(expected, ss.str());
}

}  // namespace

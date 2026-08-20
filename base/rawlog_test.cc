#include <sstream>

#include "base/basic_test.h"
#include "base/rawlog.h"

namespace {

TEST(LogInfoTest) {
  std::stringstream ss;
  ::base::rawlog::TESTONLY_SetInfoStream(&ss);
  RAW_INFO << "Hello " << "Everyone!";
  ::base::rawlog::TESTONLY_SetInfoStream(nullptr);
  std::string expected = "I base/rawlog_test.cc:11] Hello Everyone!\n";
  EXPECT_EQ(expected, ss.str());
}

TEST(LogErrorTest) {
  std::stringstream ss;
  ::base::rawlog::TESTONLY_SetErrorStream(&ss);
  RAW_ERROR << "Oh no I had an error :(";
  ::base::rawlog::TESTONLY_SetErrorStream(nullptr);
  std::string expected = "E base/rawlog_test.cc:20] Oh no I had an error :(\n";
  EXPECT_EQ(expected, ss.str());
}

volatile int global_aborts = 0;
void AbortHandler(int sig) { global_aborts = global_aborts + 1; }

TEST(CheckFaultTest) {
  struct sigaction s = {};
  s.sa_handler = &AbortHandler;
  sigemptyset(&s.sa_mask);
  s.sa_flags = 0;
  sigaction(SIGABRT, &s, nullptr);

  RAW_CHECK(7 > 8) << "This is a check falure.";
  ASSERT_EQ(1, global_aborts);
}

}  // namespace

#include "capsule/compiler.h"
#include "testing/testing.h"

using testing::HasSubstr;

namespace {

class NegativeCompilationFixture : public ::testing::Test {
 protected:
  Result RunCompilerOn(std::string_view file_name) {
    return capsule::VerifyFrontendCompile(file_name);
  }

  void ExpectComplaintAbout(Result r, std::string_view stuff) {
    EXPECT_THAT(r.ToString(), HasSubstr(stuff));
  }

  void Nope(std::string_view f, std::string_view complaint) {
    std::string full = "capsule/testdata/";
    full += f;
    EXPECT_THAT(RunCompilerOn(full).ToString(), HasSubstr(complaint));
  }
};

TEST_F(NegativeCompilationFixture, RequiresClosingBraces) {
  Nope("no_closing_brace.capsule",  //
       "6: Error: Expected '}' at end of capsule");
}

TEST_F(NegativeCompilationFixture, RequiresAtLeastOneCapsule) {
  Nope("no_capsules.capsule",  //
       "0: No capsules defined.");
}

}  // namespace

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

TEST_F(NegativeCompilationFixture, NonBinaryFile) {
  Nope("small_binary.capsule",  //
       "kStreamFatal");         // not too opinionated
}

TEST_F(NegativeCompilationFixture, NoNamespace) {
  Nope("no_namespace.capsule",  //
       "Expected 'namespace' declaration");
}

TEST_F(NegativeCompilationFixture, MultipleNamespaces) {
  Nope("two_namespaces.capsule",  //
       "9: Error: Expected 'capsule' keyword");
}

TEST_F(NegativeCompilationFixture, BogusType) {
  Nope("bogus_type.capsule",  //
       "5: Unrecognized primitive, vector, or capsule type [u65]");
}

TEST_F(NegativeCompilationFixture, BogusVariableNameSameAsPrimitiveType) {
  Nope("type_named_variable.capsule",  //
       "6: Fields may not use a primitive typename as a name [u64]");
}

TEST_F(NegativeCompilationFixture, BogusVariableNameSameAsCapsule) {
  Nope(
      "field_name_same_as_capsule.capsule",  //
      "6: Fields may not share a name with a capsule in the same file [Cappy]");
}

TEST_F(NegativeCompilationFixture, BogusCapsuleNameSameAsPrimitive) {
  Nope("capsule_named_like_primitive.capsule",  //
       "3: Capsules may not use a primitive typename as a name [bool]");
}

}  // namespace

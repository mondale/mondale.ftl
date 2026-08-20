#include "base/basic_test.h"
#include "core/file.h"

using namespace core;
using namespace std::string_literals;

namespace {

TEST(GetAndSetContents) {
  constexpr char kFile[] = "/tmp/file_test_temp_file.txt";
  const auto r = WriteContentsToFile(kFile, "It's beautiful!");
  ASSERT_TRUE(r.IsOk());
  const auto contents = ReadContentsFromFile(kFile).ValueOrDie();
  EXPECT_EQ(contents, "It's beautiful!"s);
}

}  // namespace

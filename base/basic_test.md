# Basic Test

Basic Test is a tiny library to facilitate testing of the lowest-level
components of `mondale.ftl`. Only components in `//base` and `//core` may use
Basic Test. Other components should use `//testing`.

The only file to include is `base/basic_test.h`. `BUILD` files should link
against `//base/basic_test` in the `deps` section.

The behavior of Basic Test is illustrated in the code example below:
```
#include "base/basic_test.h"  // Basic Test header
#include "path/to/thing/to/test.h"  // Other includes as needed

// Any namespace except `base::testing` may be used. Select the namespace that
// makes the test code most readable if testing within a single target
// namespace, or use an anonymous namespace if testing across multple target
// namespaces or if the choice of namespace doesn't assist test readability.
namespace {

TEST(ThisIsAFailingTest) {  // A test case called ThisIsAFailingTest
  ADD_FAILURE() << "Explicitly add a test failure at this location.";
}  // End of test case.

TEST(ThisIsAPassingTest) {
  // The EXPECT family of macros raises an error if the requested condition
  // is untrue and proceeds with the rest of the test.
  EXPECT_TRUE(true);  // Always a boolean argument.
  EXPECT_FALSE(0 == 1);
  EXPECT_EQ(7, 7);  // Must use the same type for both arguments.
  EXPECT_NE(6, 7);
  EXPECT_LT(6, 7);
  EXPECT_LE(6, 6);
  EXPECT_GT(7, 6);
  EXPECT_GE(7, 7);
  
  // The ASSERT family of macros halt the test execution at the site of the
  // first failed ASSERT.
  ASSERT_TRUE(true);
  ASSERT_FALSE(0 == 1);
  ASSERT_EQ(7, 7);
  ASSERT_NE(6, 7);
  ASSERT_LT(6, 7);
  ASSERT_LE(6, 6);
  ASSERT_GT(7, 6);
  ASSERT_GE(7, 7);
}

}  // namespace
```
The example above is a complete file. Do not include a `main()` in test
definitions as this is provided by the `//base:basic_test` target.

The set of expectations is intentionally minimal in Basic Test. Streaming
outputs to macros is not supported except for the `ADD_FAILURE()` macro.

All failed expectations and assertions output the file and line number
associated with the failure.

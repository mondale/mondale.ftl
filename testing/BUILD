
cc_library(
  name = "testing",
  hdrs = ["testing.h"],
  srcs = ["test_main.cc",],
  deps = [":test",
          ":death",
          ":matchers",
          ":mock",
          "//base:process"],
  visibility = ["//...", ],
)

cc_library(
  name = "helpers",
  hdrs = ["helpers.h"],
  srcs = [],
  deps = [":scoped_temp_file"],
  visibility = ["//...", ],
)

cc_library(
  name = "compare",
  hdrs = ["compare.h"],
  srcs = [],
  deps = [],
)

cc_test(
  name = "compare_test",
  srcs = ["compare_test.cc"],
  deps = [":compare",
          ":testing",],
)

cc_library(
  name = "death",
  hdrs = ["death.h"],
  srcs = ["death.cc"],
  deps = [],
)

cc_library(
  name = "death_test_subprocess",
  hdrs = ["death_test_subprocess.h"],
  srcs = ["death_test_subprocess.cc"],
  deps = [":death"],
)

cc_library(
  name = "expect_helper",
  hdrs = ["expect_helper.h"],
  srcs = ["expect_helper.cc"],
  deps = [":compare",
          ":death",
          ":death_test_subprocess",
  ],
)

cc_library(
  name = "matchers",
  hdrs = ["matchers.h"],
  srcs = ["matchers.cc"],
  deps = [],
)

cc_test(
  name = "matchers_test",
  srcs = ["matchers_test.cc"],
  deps = [":testing",],
)

cc_library(
  name = "mock",
  hdrs = ["mock.h"],
  srcs = [],
  deps = [":test",],
)

cc_test(
  name = "mock_test",
  srcs = ["mock_test.cc"],
  deps = [":testing"],
)

cc_library(
  name = "scoped_temp_file",
  hdrs = ["scoped_temp_file.h"],
  srcs = ["scoped_temp_file.cc"],
  deps = [],
)

cc_test(
  name = "scoped_temp_file_test",
  srcs = ["scoped_temp_file_test.cc"],
  deps = [":scoped_temp_file",
          ":testing"],
)

cc_library(
  name = "test",
  hdrs = ["test.h"],
  srcs = ["test.cc"],
  deps = [":expect_helper"],
)

cc_test(
  name = "test_test",
  srcs = ["test_test.cc"],
  deps = [":testing",],
)

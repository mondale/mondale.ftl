
cc_library(
  name = "testing",
  hdrs = ["testing.h"],
  srcs = ["test_main.cc",],
  deps = [":test",
          ":death",
          ":matchers",
          "//base:process"],
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
  name = "expectation",
  hdrs = ["expectation.h"],
  srcs = ["expectation.cc"],
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
  name = "test",
  hdrs = ["test.h"],
  srcs = ["test.cc"],
  deps = [":expectation"],
)

cc_test(
  name = "test_test",
  srcs = ["test_test.cc"],
  deps = [":testing",],
)

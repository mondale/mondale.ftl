
cc_library(
  name = "testing",
  hdrs = ["testing.h"],
  srcs = ["test_main.cc",],
  deps = [":test",
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
  name = "expectation",
  hdrs = ["expectation.h"],
  srcs = ["expectation.cc"],
  deps = [":compare"]
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

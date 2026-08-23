
cc_library(
  name = "testing",
  hdrs = ["testing.h"],
  srcs = ["test_main.cc",],
  deps = [":test",
          "//base:process"],
  visibility = ["//...", ],
)

cc_library(
  name = "test",
  hdrs = ["test.h"],
  srcs = ["test.cc"],
)

cc_test(
  name = "test_test",
  srcs = ["test_test.cc"],
  deps = [":testing",],
)

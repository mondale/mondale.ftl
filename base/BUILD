
cc_library(
  name = "basic_test",
  hdrs = ["basic_test.h"],
  srcs = ["basic_test.cc"],
)

cc_test(
  name = "basic_test_test",
  deps = [":basic_test",],
  srcs = ["basic_test_test.cc"],
)

cc_binary(
  name = "empty_main",
  srcs = ["empty_main.cc"],
)

cc_test(
  name = "empty_test",
  deps = [":basic_test",],
  srcs = ["empty_test.cc"],
)

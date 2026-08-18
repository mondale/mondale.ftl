
cc_library(
  name = "result",
  hdrs = ["result.h"],
  srcs = ["result.cc"],
  deps = ["//base:rawlog",],
)

cc_test(
  name = "result_test",
  srcs = ["result_test.cc"],
  deps = [":result",
          "//base:basic_test",
          "//base:rawlog",
  ],
)

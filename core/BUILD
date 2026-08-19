
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

cc_library(
  name = "strings",
  hdrs = ["strings.h"],
  srcs = ["strings.cc"],
  deps = [":result"],
  compiler_flags = ["-fno-exceptions"],
)

cc_test(
  name = "strings_test",
  srcs = ["strings_test.cc"],
  deps = [":strings",
          "//base:basic_test",
          "//base:rawlog",],
)

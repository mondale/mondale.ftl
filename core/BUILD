cc_library(
  name = "handle",
  hdrs = ["handle.h"],
  deps = []
)

cc_test(
  name = "handle_test",
  srcs = ["handle_test.cc"],
  deps = [":handle",
          "//base:basic_test",]
)

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

cc_library(
  name = "util",
  hdrs = ["util.h"],
  srcs = ["util.cc"],
  deps = [":result"],
  compiler_flags = ["-fno-exceptions"],
)

cc_test(
  name = "util_test",
  srcs = ["util_test.cc"],
  deps = [":util",
          "//base:basic_test",
          "//base:rawlog",],
)

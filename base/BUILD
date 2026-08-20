
cc_library(
  name = "async_safe",
  hdrs = ["async_safe.h"],
  srcs = ["async_safe.cc"],
  deps = [":raw_syscalls"],
)

cc_test(
  name = "async_safe_test",
  srcs = ["async_safe_test.cc"],
  deps = [":async_safe",
          ":basic_test",],
)

cc_library(
  name = "basic_test",
  hdrs = ["basic_test.h"],
  srcs = ["basic_test.cc",
          "basic_test_main.cc"],
  deps = [":process"],
  visibility = ["//base/...",
                "//core/...",],
)

cc_test(
  name = "basic_test_test",
  deps = [":basic_test"],
  srcs = ["basic_test_test.cc"],
)

cc_binary(
  name = "empty_main",
  srcs = ["empty_main.cc"],
)

cc_binary(
  name = "claude_stacktrace_with_lines",
  srcs = ["claude_stacktrace_with_lines.cc"],
)

cc_library(
  name = "process",
  hdrs = ["process.h"],
  srcs = ["process.cc"],
  deps = [":async_safe",
          ":raw_syscalls",
          ":stacktrace",
  ],
)

cc_library(
  name = "rawlog",
  hdrs = ["rawlog.h"],
  srcs = ["rawlog.cc"],
  visibility = ["//base/...",
                "//core/...",],
)

cc_test(
  name = "rawlog_test",
  srcs = ["rawlog_test.cc"],
  deps = [":rawlog",
          ":basic_test",],
)

cc_library(
  name = "raw_syscalls",
  hdrs = ["raw_syscalls.h"],
  srcs = ["raw_syscalls.cc"],
)

cc_test(
  name = "raw_syscalls_test",
  srcs = ["raw_syscalls_test.cc"],
  deps = [":basic_test",
          ":raw_syscalls"],
)

cc_binary(
  name = "segfaulter",
  srcs = ["segfaulter.cc"],
  deps = [":process",]
)

cc_library(
  name = "source_location",
  hdrs = ["source_location.h"],
  deps = [])

cc_test(
  name = "source_location_test",
  srcs = ["source_location_test.cc"],
  deps = [":source_location",
          ":basic_test",]
)

cc_library(
  name = "stacktrace",
  hdrs = ["stacktrace.h"],
  srcs = ["stacktrace.cc"],
  deps = [":async_safe",
          ":raw_syscalls"],
)

cc_test(
  name = "stacktrace_test",
  srcs = ["stacktrace_test.cc"],
  deps = [":stacktrace",
          ":basic_test"],
)

cc_library(
  name = "time",
  hdrs = ["time.h"],
  srcs = ["time.cc"],
  deps = [":rawlog"]
)

cc_test(
  name = "time_test",
  srcs = ["time_test.cc"],
  deps = [":time",
          ":basic_test"],
)
  


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
          "//testing",],
)

cc_library(
  name = "mutex",
  hdrs = ["mutex.h"],
  srcs = ["mutex.cc"],
  deps = [":thread_annotations",],
  visibility = ["//..."],
)

cc_test(
  name = "mutex_test",
  srcs = ["mutex_test.cc"],
  deps = [":mutex",
          "//core",
          "//testing"],
)

cc_library(
  name = "notification",
  hdrs = ["notification.h"],
  srcs = ["notification.cc"],
  deps = [":time"],
  visibility = ["//..."],
)

cc_test(
  name = "notification_test",
  srcs = ["notification_test.cc"],
  deps = [":notification",
          "//core",
          "//testing"],
)


cc_library(
  name = "process",
  hdrs = ["process.h"],
  srcs = ["process.cc"],
  deps = [":async_safe",
          ":raw_syscalls",
          ":stacktrace",
  ],
  visibility = ["//..."],
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
          "//testing",],
)

cc_library(
  name = "raw_syscalls",
  hdrs = ["raw_syscalls.h"],
  srcs = ["raw_syscalls.cc"],
)

cc_test(
  name = "raw_syscalls_test",
  srcs = ["raw_syscalls_test.cc"],
  deps = [":raw_syscalls",
          "//testing",],
)

cc_binary(
  name = "segfaulter",
  srcs = ["segfaulter.cc"],
  deps = [":process",]
)

cc_library(
  name = "sleep",
  hdrs = ["sleep.h"],
  srcs = ["sleep.cc"],
  deps = [":time",],
  visibility = ["//base/...",
                "//core"],
)

cc_test(
  name = "sleep_test",
  srcs = ["sleep_test.cc"],
  deps = [":sleep",
          ":time",
          "//testing",
         ]
)

cc_library(
  name = "source_location",
  hdrs = ["source_location.h"],
  deps = [])

cc_test(
  name = "source_location_test",
  srcs = ["source_location_test.cc"],
  deps = [":source_location",
          "//testing",],
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
          "//testing",],
)

cc_library(
    name = "thread",
    hdrs = ["thread.h"],
    srcs = ["thread.cc"],
    deps = [],
    visibility = ["//..."],
)

cc_test(
    name = "thread_test",
    srcs = ["thread_test.cc"],
    deps = [":thread",
            "//core",
            "//testing",
           ],
)

cc_library(
    name = "thread_annotations",
    hdrs = ["thread_annotations.h"],
    srcs = [],
    deps = [],
    visibility = ["//..."],
)

cc_library(
  name = "time",
  hdrs = ["time.h"],
  srcs = ["time.cc"],
  deps = [":rawlog"],
  visibility = ["//base/...",
                "//core"],
)

cc_test(
  name = "time_test",
  srcs = ["time_test.cc"],
  deps = [":time",
          "//testing",],
)
  

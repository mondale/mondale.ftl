
cc_binary(
  name = "webdump",
  srcs = ["webdump.cc"],
  deps = [
    "//base:process",
    "//core",
    "//io:epoller",
    "//io:context",
    "//io:data_handler",
    "//io:sink_buffer",
  ],
)

cc_library(
  name = "listener",
  hdrs = ["listener.h"],
  srcs = ["listener.cc"],
  deps = ["//core", "//io:io_handler",],
)

cc_test(
  name = "listener_test",
  srcs = ["listener_test.cc"],
  deps = [":listener",
          "//testing",
  ],
)

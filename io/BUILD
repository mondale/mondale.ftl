cc_library(
    name = "epoller",
    srcs = ["epoller.cc"],
    hdrs = ["epoller.h"],
    deps = [
        ":io_handler",
        ":silo",
        "//core",
        "//core:file_descriptor",
        "//core:handle_table",
        "//core:stateless_random",
    ],
)

cc_test(
    name = "epoller_test",
    srcs = ["epoller_test.cc"],
    deps = [
        ":epoller",
        "//testing",
    ],
)

cc_library(
  name = "silo",
  hdrs = ["silo.h"],
  srcs = ["silo.cc"],
  deps = ["//core", ":io_handler",],
)

cc_test(
  name = "silo_test",
  srcs = ["silo_test.cc"],
  deps = [":silo",
          "//testing",
  ],
)


cc_library(
  name = "io_handler",
  hdrs = ["io_handler.h"],
  srcs = ["io_handler.cc"],
  deps = ["//core","//core:file_descriptor",],
)

cc_test(
  name = "io_handler_test",
  srcs = ["io_handler_test.cc"],
  deps = [":io_handler",
          "//testing",
  ],
)

cc_binary(
    name = "webdump",
    srcs = ["webdump.cc"],
    deps = [
        "//base:process",
        "//net:listener",
        "//core",
        "//io",
    ],
)

cc_library(
  name = "web_handler",
  hdrs = ["web_handler.h"],
  srcs = ["web_handler.cc"],
  deps = ["//core",
"//io",
  ],
)

cc_test(
  name = "web_handler_test",
  srcs = ["web_handler_test.cc"],
  deps = [":web_handler",
          "//testing",
  ],
)

cc_library(
    name = "listener",
    srcs = ["listener.cc"],
    hdrs = ["listener.h"],
    deps = [
        ":socket_address",
        "//core",
        "//core:idioms",
        "//io",
    ],
    visibility = ["//http/..."],
)

cc_test(
    name = "listener_test",
    srcs = ["listener_test.cc"],
    deps = [
        ":listener",
        ":socket_address",
        "//io",
        "//testing",
    ],
)

cc_library(
    name = "socket_address",
    srcs = ["socket_address.cc"],
    hdrs = ["socket_address.h"],
    deps = ["//core"],
    visibility = ["//http/..."],
)

cc_test(
    name = "socket_address_test",
    srcs = ["socket_address_test.cc"],
    deps = [
        ":socket_address",
        "//testing",
    ],
)

cc_library(
  name = "ssl_context",
  hdrs = ["ssl_context.h", "keys/server.crt", "keys/server.key",],
  srcs = ["ssl_context.cc"],
  deps = ["//core",
  ],
  linker_flags = ["-lssl", "-lcrypto"],
  copts = ["-Wno-c23-extensions"],
)

cc_test(
  name = "ssl_context_test",
  srcs = ["ssl_context_test.cc"],
  deps = [":ssl_context",
          "//testing",
  ],
)

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
":ssl_helpers",
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

cc_library(
  name = "ssl_socket",
  hdrs = ["ssl_socket.h"],
  srcs = ["ssl_socket.cc"],
  linker_flags = ["-lssl", "-lcrypto"],
  deps = ["//core",
"//io",
":socket_address",
":ssl_context",
":ssl_handshaker",
  ],
)

cc_test(
  name = "ssl_socket_test",
  srcs = ["ssl_socket_test.cc"],
  deps = [":ssl_socket",
          "//testing",
  ],
)

cc_library(
  name = "ssl_handshaker",
  hdrs = ["ssl_handshaker.h"],
  srcs = ["ssl_handshaker.cc"],
  linker_flags = ["-lssl", "-lcrypto"],
  deps = ["//core",
":ssl_helpers",
  ],
)

cc_test(
  name = "ssl_handshaker_test",
  srcs = ["ssl_handshaker_test.cc"],
  deps = [":ssl_handshaker",
          "//testing",
  ],
)

cc_library(
  name = "ssl_helpers",
  hdrs = ["ssl_helpers.h"],
  srcs = ["ssl_helpers.cc"],
  linker_flags = ["-lssl", "-lcrypto"],
  deps = ["//core",
  ],
)

cc_test(
  name = "ssl_helpers_test",
  srcs = ["ssl_helpers_test.cc"],
  deps = [":ssl_helpers",
          "//testing",
  ],
)

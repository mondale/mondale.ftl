cc_library(
    name = "listener",
    srcs = ["listener.cc"],
    hdrs = ["listener.h"],
    visibility = ["//http/..."],
    deps = [
        ":socket_address",
        "//core",
        "//core:idioms",
        "//io",
    ],
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
    visibility = ["//http/..."],
    deps = ["//core"],
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
    srcs = ["ssl_context.cc"],
    hdrs = [
        "keys/server.crt",
        "keys/server.key",
        "ssl_context.h",
    ],
    copts = ["-Wno-c23-extensions"],
    linker_flags = [
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        ":ssl_helpers",
        "//core",
    ],
)

cc_test(
    name = "ssl_context_test",
    srcs = ["ssl_context_test.cc"],
    deps = [
        ":ssl_context",
        "//testing",
    ],
)

cc_library(
    name = "ssl_socket",
    srcs = ["ssl_socket.cc"],
    hdrs = ["ssl_socket.h"],
    linker_flags = [
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        ":socket_address",
        ":ssl_context",
        ":ssl_handshaker",
        "//core",
        "//io",
    ],
)

cc_test(
    name = "ssl_socket_test",
    srcs = ["ssl_socket_test.cc"],
    deps = [
        ":ssl_socket",
        "//testing",
    ],
)

cc_library(
    name = "ssl_handshaker",
    srcs = ["ssl_handshaker.cc"],
    hdrs = ["ssl_handshaker.h"],
    linker_flags = [
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        ":ssl_helpers",
        "//core",
    ],
)

cc_test(
    name = "ssl_handshaker_test",
    srcs = ["ssl_handshaker_test.cc"],
    deps = [
        ":ssl_handshaker",
        "//testing",
    ],
)

cc_library(
    name = "ssl_helpers",
    srcs = ["ssl_helpers.cc"],
    hdrs = ["ssl_helpers.h"],
    linker_flags = [
        "-lssl",
        "-lcrypto",
    ],
    deps = [
        "//core",
    ],
)

cc_test(
    name = "ssl_helpers_test",
    srcs = ["ssl_helpers_test.cc"],
    deps = [
        ":ssl_helpers",
        "//testing",
    ],
)

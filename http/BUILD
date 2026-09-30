cc_binary(
    name = "webdump",
    srcs = ["webdump.cc"],
    deps = [
        ":request_parser",
        "//base:process",
        "//core",
        "//io",
        "//net:listener",
        "//net:ssl_context",
        "//net:ssl_socket",
        "//net:tls_sniffer",
    ],
)

cc_library(
    name = "web_socket",
    srcs = ["web_socket.cc"],
    hdrs = ["web_socket.h"],
    deps = [
        "//core",
        "//io",
        "//net:socket_address",
    ],
)

cc_test(
    name = "web_socket_test",
    srcs = ["web_socket_test.cc"],
    deps = [
        ":web_socket",
        "//testing",
    ],
)

cc_library(
    name = "request_parser",
    srcs = ["request_parser.cc"],
    hdrs = ["request_parser.h"],
    deps = [
        "//core",
    ],
)

cc_test(
    name = "request_parser_test",
    srcs = ["request_parser_test.cc"],
    deps = [
        ":request_parser",
        "//testing",
    ],
)

cc_library(
    name = "http_stream",
    srcs = ["http_stream.cc"],
    hdrs = ["http_stream.h"],
    deps = [
        ":request_parser",
        "//core",
        "//io",
    ],
)

cc_test(
    name = "http_stream_test",
    srcs = ["http_stream_test.cc"],
    deps = [
        ":http_stream",
        "//io",
        "//testing",
    ],
)

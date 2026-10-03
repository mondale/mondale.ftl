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

cc_binary(
    name = "webserver",
    srcs = ["webserver_main.cc"],
    deps = [
        ":http_server",
        "//base:process",
        "//core",
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

cc_library(
    name = "response_builder",
    srcs = ["response_builder.cc"],
    hdrs = ["response_builder.h"],
    deps = [
        "//core",
    ],
)

cc_test(
    name = "response_builder_test",
    srcs = ["response_builder_test.cc"],
    deps = [
        ":response_builder",
        "//testing",
    ],
)

cc_library(
    name = "http_server",
    srcs = ["http_server.cc"],
    hdrs = ["http_server.h"],
    deps = [
        ":http_stream",
        ":request_parser",
        ":web_socket",
        "//core",
        "//io",
        "//net:listener",
        "//net:socket_address",
        "//net:ssl_context",
        "//net:ssl_socket",
        "//net:tls_sniffer",
    ],
)

cc_test(
    name = "http_server_test",
    srcs = ["http_server_test.cc"],
    deps = [
        ":http_server",
        "//testing",
    ],
)

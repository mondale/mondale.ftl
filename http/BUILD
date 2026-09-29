cc_binary(
    name = "webdump",
    srcs = ["webdump.cc"],
    deps = [
        ":request_parser",
        "//base:process",
        "//core",
        "//io",
        "//net:listener",
    ],
)

cc_library(
    name = "web_handler",
    srcs = ["web_handler.cc"],
    hdrs = ["web_handler.h"],
    deps = [
        "//core",
        "//io",
    ],
)

cc_test(
    name = "web_handler_test",
    srcs = ["web_handler_test.cc"],
    deps = [
        ":web_handler",
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

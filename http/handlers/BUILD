cc_library(
    name = "statusz",
    srcs = ["statusz.cc"],
    hdrs = ["statusz.h"],
    visibility = ["//..."],
    deps = [
        "//core",
        "//http:http_server",
        "//http:request_parser",
        "//http:response_builder",
    ],
    alwayslink = True,
)

cc_test(
    name = "statusz_test",
    srcs = ["statusz_test.cc"],
    deps = [
        ":statusz",
        "//testing",
    ],
)

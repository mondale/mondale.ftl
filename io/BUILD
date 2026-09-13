cc_library(
    name = "epoller",
    srcs = ["epoller.cc"],
    hdrs = ["epoller.h"],
    deps = [
        "//core",
        "//core:file_descriptor",
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

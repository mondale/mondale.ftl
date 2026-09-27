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
)

cc_test(
    name = "socket_address_test",
    srcs = ["socket_address_test.cc"],
    deps = [
        ":socket_address",
        "//testing",
    ],
)

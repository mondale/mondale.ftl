cc_library(
    name = "epoller",
    srcs = ["epoller.cc"],
    hdrs = ["epoller.h"],
    deps = [
        ":io_handler",
        ":silo",
        "//core",
        "//core:file_descriptor",
        "//core:handle_table",
        "//core:idioms",
        "//core:stateless_random",
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

cc_library(
    name = "silo",
    srcs = ["silo.cc"],
    hdrs = ["silo.h"],
    deps = [
        ":context",
        ":io_handler",
        ":utilization_estimator",
        "//core",
        "//core:handle_table",
        "//core:intrusive_list",
        "//core:syscalls",
    ],
)

cc_test(
    name = "silo_test",
    srcs = ["silo_test.cc"],
    deps = [
        ":silo",
        ":test_handlers",
        "//core",
        "//core:idioms",
        "//core:syscalls",
        "//testing",
    ],
)

cc_library(
    name = "io_handler",
    srcs = ["io_handler.cc"],
    hdrs = ["io_handler.h"],
    deps = [
        "//core",
        "//core:file_descriptor",
        "//core:idioms",
        "//core:inlined_vector",
        "//core:syscalls",
    ],
)

cc_test(
    name = "io_handler_test",
    srcs = ["io_handler_test.cc"],
    deps = [
        ":io_handler",
        "//testing",
    ],
)

cc_library(
    name = "utilization_estimator",
    srcs = [],
    hdrs = ["utilization_estimator.h"],
    deps = [],
)

cc_test(
    name = "utilization_estimator_test",
    srcs = ["utilization_estimator_test.cc"],
    deps = [
        ":utilization_estimator",
        "//testing",
    ],
)

cc_library(
    name = "context",
    srcs = [],
    hdrs = ["context.h"],
    deps = [
        ":io_handler",
        "//base:time",
    ],
)

cc_test(
    name = "context_test",
    srcs = ["context_test.cc"],
    deps = [
        ":context",
        "//testing",
    ],
)

cc_library(
    name = "test_handlers",
    srcs = ["test_handlers.cc"],
    hdrs = ["test_handlers.h"],
    deps = [
        ":context",
        ":io_handler",
        "//core",
    ],
)

cc_test(
    name = "test_handlers_test",
    srcs = ["test_handlers_test.cc"],
    deps = [
        ":test_handlers",
        "//testing",
    ],
)

cc_library(
    name = "core",
    hdrs = ["vocabulary.h"],
    srcs = [],
    deps = [
         ":handle",
         ":hardened_int",
         ":result",
         ":util",
         "//base:mutex",
         "//base:sleep",
         "//base:thread",
         "//base:thread_annotations",
         "//base:time",
    ],
    visibility = ["//..."],
)

cc_library(
    name = "file",
    srcs = ["file.cc"],
    hdrs = ["file.h"],
    deps = [
        ":idioms",
        ":result",
        ":strings",
        ":syscalls",
    ],
)

cc_test(
    name = "file_test",
    srcs = ["file_test.cc"],
    deps = [
        ":file",
        "//testing",
    ],
)

cc_library(
    name = "file_descriptor",
    srcs = ["file_descriptor.cc"],
    hdrs = ["file_descriptor.h"],
    deps = [
        ":result",
        "//base:rawlog",
    ],
)

cc_test(
    name = "file_descriptor_test",
    srcs = ["file_descriptor_test.cc"],
    deps = [
        ":file_descriptor",
        "//testing",
    ],
)

cc_library(
    name = "handle",
    hdrs = ["handle.h"],
    deps = [],
)

cc_test(
    name = "handle_test",
    srcs = ["handle_test.cc"],
    deps = [
        ":handle",
        "//testing",
    ],
)

cc_library(
    name = "hardened_int",
    hdrs = ["hardened_int.h"],
    deps = [],
)

cc_test(
    name = "hardened_int_test",
    srcs = ["hardened_int_test.cc"],
    deps = [
        ":hardened_int",
        "//testing",
    ],
)

cc_library(
    name = "idioms",
    srcs = ["idioms.cc"],
    hdrs = ["idioms.h"],
    deps = [":syscalls"],
)

cc_library(
    name = "result",
    srcs = ["result.cc"],
    hdrs = ["result.h"],
    deps = ["//base:rawlog"],
)

cc_test(
    name = "result_test",
    srcs = ["result_test.cc"],
    deps = [
        ":result",
        "//base:rawlog",
        "//testing",
    ],
)

cc_library(
    name = "strings",
    srcs = ["strings.cc"],
    hdrs = ["strings.h"],
    compiler_flags = ["-fno-exceptions"],
    deps = [":result"],
)

cc_test(
    name = "strings_test",
    srcs = ["strings_test.cc"],
    deps = [
        ":strings",
        "//base:rawlog",
        "//testing",
    ],
)

cc_library(
    name = "syscalls",
    srcs = ["syscalls.cc"],
    hdrs = ["syscalls.h"],
    deps = [
        ":file_descriptor",
        ":result",
        "//base:rawlog",
    ],
)

cc_test(
    name = "syscalls_test",
    srcs = ["syscalls_test.cc"],
    deps = [
        ":syscalls",
        "//testing",
    ],
)

cc_library(
    name = "util",
    srcs = ["util.cc"],
    hdrs = ["util.h"],
    compiler_flags = ["-fno-exceptions"],
    deps = [":result"],
)

cc_test(
    name = "util_test",
    srcs = ["util_test.cc"],
    deps = [
        ":util",
        "//base:rawlog",
        "//testing",
    ],
)

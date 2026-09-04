cc_library(
    name = "testing",
    srcs = ["test_main.cc"],
    hdrs = ["testing.h"],
    visibility = ["//..."],
    deps = [
        ":death",
        ":matchers",
        ":mock",
        ":test",
        "//base:process",
    ],
)

cc_library(
    name = "helpers",
    srcs = [],
    hdrs = ["helpers.h"],
    visibility = ["//..."],
    deps = [":scoped_temp_file"],
)

cc_library(
    name = "compare",
    srcs = [],
    hdrs = ["compare.h"],
    deps = [],
)

cc_test(
    name = "compare_test",
    srcs = ["compare_test.cc"],
    deps = [
        ":compare",
        ":testing",
    ],
)

cc_library(
    name = "death",
    srcs = ["death.cc"],
    hdrs = ["death.h"],
    deps = [],
)

cc_library(
    name = "death_test_subprocess",
    srcs = ["death_test_subprocess.cc"],
    hdrs = ["death_test_subprocess.h"],
    deps = [":death"],
)

cc_library(
    name = "expect_helper",
    srcs = ["expect_helper.cc"],
    hdrs = ["expect_helper.h"],
    deps = [
        ":compare",
        ":death",
        ":death_test_subprocess",
    ],
)

cc_library(
    name = "matchers",
    srcs = ["matchers.cc"],
    hdrs = ["matchers.h"],
    deps = [],
)

cc_test(
    name = "matchers_test",
    srcs = ["matchers_test.cc"],
    deps = [":testing"],
)

cc_library(
    name = "mock",
    srcs = [],
    hdrs = ["mock.h"],
    deps = [":test"],
)

cc_test(
    name = "mock_test",
    srcs = ["mock_test.cc"],
    deps = [":testing"],
)

cc_library(
    name = "scoped_temp_file",
    srcs = ["scoped_temp_file.cc"],
    hdrs = ["scoped_temp_file.h"],
    deps = [],
)

cc_test(
    name = "scoped_temp_file_test",
    srcs = ["scoped_temp_file_test.cc"],
    deps = [
        ":scoped_temp_file",
        ":testing",
    ],
)

cc_library(
    name = "test",
    srcs = ["test.cc"],
    hdrs = ["test.h"],
    deps = [
        ":expect_helper",
        "//base:process",
    ],
)

cc_test(
    name = "test_test",
    srcs = ["test_test.cc"],
    deps = [":testing"],
)

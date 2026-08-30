cc_library(
    name = "ast",
    srcs = [],
    hdrs = ["ast.h"],
)

cc_library(
    name = "generator",
    srcs = ["generator.cc"],
    hdrs = ["generator.h"],
    deps = [
        ":ast",
        "//core",
    ],
)

cc_test(
    name = "generator_smoke_test",
    srcs = ["generator_smoke_test.cc"],
    deps = [
        ":generator",
        ":parser",
        "//testing",
    ],
)

cc_library(
    name = "lexer",
    srcs = ["lexer.cc"],
    hdrs = ["lexer.h"],
    deps = [
        ":ast",
        "//core",
    ],
)

cc_library(
    name = "parser",
    srcs = ["parser.cc"],
    hdrs = ["parser.h"],
    deps = [
        ":ast",
        ":lexer",
        "//core",
    ],
)

cc_library(
    name = "storage",
    srcs = ["storage.cc"],
    hdrs = ["storage.h"],
    deps = [],
)

cc_test(
    name = "storage_test",
    srcs = ["storage_test.cc"],
    deps = [
        ":storage",
        "//testing",
    ],
)

cc_library(
    name = "storage_factory",
    srcs = ["storage_factory.cc"],
    hdrs = ["storage_factory.h"],
    deps = ["//core"],
)

cc_test(
    name = "storage_factory_test",
    srcs = ["storage_factory_test.cc"],
    deps = [
        ":storage_factory",
        "//core",
        "//testing",
    ],
)

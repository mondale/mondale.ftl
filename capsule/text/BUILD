cc_library(
    name = "text_lexer",
    srcs = ["text_lexer.cc"],
    hdrs = ["text_lexer.h"],
    deps = ["//core"],
)

cc_test(
    name = "text_lexer_test",
    srcs = ["text_lexer_test.cc"],
    deps = [
        ":text_lexer",
        "//testing",
    ],
)

cc_library(
    name = "text_parser",
    srcs = ["text_parser.cc"],
    hdrs = ["text_parser.h"],
    visibility = ["//..."],
    deps = [
        ":text_lexer",
        "//core",
    ],
)

cc_library(
    name = "parsing_widget",
    srcs = ["parsing_widget.cc"],
    hdrs = ["parsing_widget.h"],
    visibility = ["//..."],
    deps = [":text_parser"],
)

cc_test(
    name = "parsing_widget_test",
    srcs = ["parsing_widget_test.cc"],
    deps = [
        ":parsing_widget",
        "//testing",
    ],
)

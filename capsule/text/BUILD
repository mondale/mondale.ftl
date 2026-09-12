
cc_library(
  name = "text_lexer",
  hdrs = ["text_lexer.h"],
  srcs = ["text_lexer.cc"],
  deps = ["//core",],
)

cc_test(
  name = "text_lexer_test",
  srcs = ["text_lexer_test.cc"],
  deps = [":text_lexer", "//testing"],
)


cc_library(
  name = "text_parser",
  hdrs = ["text_parser.h"],
  srcs = ["text_parser.cc"],
  deps = [":text_lexer",
          "//core",],
  visibility = ["//capsule/..."],
)


cc_library(
  name = "parsing_widget",
  hdrs = ["parsing_widget.h"],
  srcs = ["parsing_widget.cc"],
  deps = [],
)

cc_test(
  name = "parsing_widget_test",
  srcs = ["parsing_widget_test.cc"],
  deps = [":parsing_widget",
          "//testing",
  ],
)

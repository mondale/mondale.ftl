
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


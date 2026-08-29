
cc_library(
 name = "ast",
 hdrs = ["ast.h"],
 srcs = [],
)

cc_library(
  name = "generator",
  hdrs = ["generator.h"],
  srcs = ["generator.cc"],
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
  hdrs = ["lexer.h"],
  srcs = ["lexer.cc"],
  deps = [
    ":ast",
    "//core",
  ]
)

cc_library(
  name = "parser",
  hdrs = ["parser.h"],
  srcs = ["parser.cc"],
  deps = [
    ":ast",
    ":lexer",
    "//core",
  ],
)

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
    deps = [
        ":storage_factory",
        "//core",
    ],
)

cc_test(
    name = "storage_test",
    srcs = ["storage_test.cc"],
    deps = [
        ":storage",
        ":storage_factory",
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

cc_library(
    name = "codec",
    srcs = ["codec.cc"],
    hdrs = ["codec.h"],
    deps = [
        ":abi",
        "//core",
        "//core:crc32c",
    ],
)

cc_test(
    name = "codec_test",
    srcs = ["codec_test.cc"],
    deps = [
        ":abi",
        ":codec",
        "//testing",
    ],
)

cc_library(
    name = "abi",
    srcs = [],
    hdrs = ["abi.h"],
    deps = ["//core:crc32c"],
)

cc_library(
    name = "view_mapper",
    srcs = ["view_mapper.cc"],
    hdrs = ["view_mapper.h"],
    deps = [
        ":abi",
        "//core",
        "//core:crc32c",
    ],
)

cc_test(
    name = "view_mapper_test",
    srcs = ["view_mapper_test.cc"],
    deps = [
        ":abi",
        ":view_mapper",
        "//core:crc32c",
        "//testing",
    ],
)

cc_library(
    name = "size_builder",
    srcs = [],
    hdrs = ["size_builder.h"],
    deps = [":abi"],
)

cc_test(
    name = "size_builder_test",
    srcs = ["size_builder_test.cc"],
    deps = [
        ":size_builder",
        "//testing",
    ],
)

cc_library(
    name = "encoder",
    srcs = ["encoder.cc"],
    hdrs = ["encoder.h"],
    deps = [
        ":codec",
        ":size_builder",
        "//core",
    ],
)

cc_test(
    name = "encoder_test",
    srcs = ["encoder_test.cc"],
    deps = [
        ":encoder",
        "//testing",
    ],
)

cc_library(
  name = "decoder",
  hdrs = ["decoder.h"],
  srcs = ["decoder.cc"],
  deps = [":abi", ":codec", ":view_mapper", "//core",],
)

cc_test(
  name = "decoder_test",
  srcs = ["decoder_test.cc"],
  deps = [":decoder",
          "//testing",
  ],
)

cc_test(
  name = "by_hand_test",
  srcs = ["by_hand_test.cc"],
  deps = [":encoder", ":decoder", "//core", ":abi", ":codec", ":size_builder", ":storage", ":storage_factory",
          "//testing",
  ],
)

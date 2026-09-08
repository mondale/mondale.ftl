subinclude("//build_defs:capsule")

cc_library(
    name = "ast",
    srcs = [],
    hdrs = ["ast.h"],
    deps = ["//core:crc32c"],
)

cc_binary(
    name = "capsule_compiler",
    srcs = ["compiler_main.cc"],
    visibility = ["//..."],
    deps = [
        ":compiler",
        "//core",
    ],
)

cc_library(
    name = "compiler",
    srcs = ["compiler.cc"],
    hdrs = ["compiler.h"],
    deps = [
        ":ast",
        ":generator",
        ":hashing",
        ":parser",
        ":verify",
        "//core",
        "//core:file",
    ],
)

cc_test(
    name = "compiler_negative_test",
    srcs = ["compiler_negative_test.cc"],
    data = [
        "testdata/bogus_type.capsule",
        "testdata/capsule_named_like_primitive.capsule",
        "testdata/dynamic_collisions.capsule",
        "testdata/field_name_same_as_capsule.capsule",
        "testdata/no_capsules.capsule",
        "testdata/no_closing_brace.capsule",
        "testdata/no_namespace.capsule",
        "testdata/redundant_capsule.capsule",
        "testdata/small_binary.capsule",
        "testdata/two_namespaces.capsule",
        "testdata/type_named_variable.capsule",
        "testdata/verboten_names.capsule",
    ],
    deps = [
        ":compiler",
        "//testing",
    ],
)

cc_test(
    name = "compiler_positive_test",
    srcs = ["compiler_positive_test.cc"],
    deps = [
        ":compiler_smoke_test_capsule",
        "//testing",
    ],
)

capsule_library(
    name = "compiler_smoke_test_capsule",
    src = "compiler_smoke_test.capsule",
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
    visibility = ["//..."],
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

capsule_library(
    name = "version_change_capsule",
    src = "version_change.capsule",
)

cc_test(
    name = "version_change_test",
    srcs = ["version_change_test.cc"],
    deps = [
        ":version_change_capsule",
        ":decoder",
        ":encoder",
        ":storage",
        ":storage_factory",
        "//core",
        "//testing",
    ],
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
    visibility = ["//..."],
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
    visibility = ["//..."],
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
    srcs = ["decoder.cc"],
    hdrs = ["decoder.h"],
    visibility = ["//..."],
    deps = [
        ":abi",
        ":codec",
        ":view_mapper",
        "//core",
    ],
)

cc_test(
    name = "decoder_test",
    srcs = ["decoder_test.cc"],
    deps = [
        ":decoder",
        "//testing",
    ],
)

cc_test(
    name = "by_hand_test",
    srcs = ["by_hand_test.cc"],
    deps = [
        ":abi",
        ":decoder",
        ":encoder",
        ":size_builder",
        ":storage",
        ":storage_factory",
        "//core",
        "//testing",
    ],
)

cc_library(
    name = "hashing",
    srcs = ["hashing.cc"],
    hdrs = ["hashing.h"],
    deps = [
        ":ast",
        "//core",
        "//core:crc32c",
    ],
)

cc_test(
    name = "hashing_test",
    srcs = ["hashing_test.cc"],
    deps = [
        ":ast",
        ":hashing",
        "//testing",
    ],
)

cc_library(
    name = "verify",
    srcs = ["verify.cc"],
    hdrs = ["verify.h"],
    deps = [
        ":ast",
        "//core",
    ],
)

cc_test(
    name = "verify_test",
    srcs = ["verify_test.cc"],
    deps = [
        ":verify",
        "//testing",
    ],
)

cc_library(
  name = "capsule",
  hdrs = ["capsule.h"],
  srcs = ["capsule.cc"],
  deps = [
          ":encoder",
          ":decoder",
          ":storage",
          "//core",
  ],
)

capsule_library(
    name = "capsule_test_capsule",
    src = "capsule_test.capsule",
)

cc_test(
  name = "capsule_test",
  srcs = ["capsule_test.cc"],
  deps = [":capsule",
          ":capsule_test_capsule",
          ":storage_factory",
          "//testing",
          "//core",
  ],
)

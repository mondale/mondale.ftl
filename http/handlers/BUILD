cc_library(
    name = "statusz",
    srcs = ["statusz.cc"],
    hdrs = ["statusz.h"],
    visibility = ["//http/..."],
    deps = [
        "//core",
        "//http:http_server",
        "//http:request_parser",
        "//http:response_builder",
    ],
    alwayslink = True,
)

cc_test(
    name = "statusz_test",
    srcs = ["statusz_test.cc"],
    deps = [
        ":statusz",
        ":test_fixture",
        "//testing",
    ],
)

cc_library(
    name = "test_fixture",
    srcs = ["test_fixture.cc"],
    hdrs = ["test_fixture.h"],
    deps = [
        "//core",
        "//http:request_parser",
        "//testing",
    ],
)

cc_test(
    name = "test_fixture_test",
    srcs = ["test_fixture_test.cc"],
    deps = [
        ":test_fixture",
        "//http:response_builder",
        "//testing",
    ],
)

cc_library(
  name = "procz",
  hdrs = ["procz.h"],
  srcs = ["procz.cc"],
  deps = ["//core",
"//core:file",
        "//http:http_server",
        "//http:request_parser",
        "//http:response_builder",
  ],
    visibility = ["//http/..."],
    alwayslink = True,
)

cc_test(
  name = "procz_test",
  srcs = ["procz_test.cc"],
  deps = [":procz",
          "//testing",
  ],
)

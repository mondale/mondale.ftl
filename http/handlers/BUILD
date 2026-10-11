
cc_library(
  name = "statusz",
  hdrs = ["statusz.h"],
  srcs = ["statusz.cc"],
  deps = ["//core",
"//http:request_parser",
"//http:response_builder",
"//http:http_server",
  ],
visibility = ["//..."],
alwayslink = True,
)

cc_test(
  name = "statusz_test",
  srcs = ["statusz_test.cc"],
  deps = [":statusz",
          "//testing",
  ],
)

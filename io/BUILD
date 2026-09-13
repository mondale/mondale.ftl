


cc_library(
  name = "epoller",
  hdrs = ["epoller.h"],
  srcs = ["epoller.cc"],
  deps = ["//core","//core:file_descriptor",],
)

cc_test(
  name = "epoller_test",
  srcs = ["epoller_test.cc"],
  deps = [":epoller",
          "//testing",
  ],
)

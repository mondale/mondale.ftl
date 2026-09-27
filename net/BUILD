cc_library(
  name = "listener",
  hdrs = ["listener.h"],
  srcs = ["listener.cc"],
  deps = ["//core", "//io", ":socket_address",
"//core:idioms",
],
)

cc_test(
  name = "listener_test",
  srcs = ["listener_test.cc"],
  deps = [":listener",
          "//io",
          "//testing",
":socket_address",
  ],
)

cc_library(
  name = "socket_address",
  hdrs = ["socket_address.h"],
  srcs = ["socket_address.cc"],
  deps = ["//core",],
)

cc_test(
  name = "socket_address_test",
  srcs = ["socket_address_test.cc"],
  deps = [":socket_address",
          "//testing",
  ],
)

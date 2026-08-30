cc_library(
    name = "async_safe",
    srcs = ["async_safe.cc"],
    hdrs = ["async_safe.h"],
    deps = [":raw_syscalls"],
)

cc_test(
    name = "async_safe_test",
    srcs = ["async_safe_test.cc"],
    deps = [
        ":async_safe",
        "//testing",
    ],
)

cc_library(
    name = "cpu",
    srcs = ["cpu.cc"],
    hdrs = ["cpu.h"],
    deps = [],
)

cc_test(
    name = "cpu_test",
    srcs = ["cpu_test.cc"],
    deps = [
        ":cpu",
        "//testing",
    ],
)

cc_library(
    name = "flags",
    srcs = ["flags.cc"],
    hdrs = ["flags.h"],
    deps = [],
)

cc_test(
    name = "flags_test",
    srcs = ["flags_test.cc"],
    deps = [
        ":flags",
        "//testing",
    ],
)

cc_library(
    name = "logging",
    srcs = ["logging.cc"],
    hdrs = ["logging.h"],
    visibility = ["//..."],
    deps = [
        ":cpu",
        ":log_writer_thread",
        ":logging_internal",
        ":source_location",
        ":thread",
        ":time",
    ],
)

cc_test(
    name = "logging_basic_test",
    srcs = ["logging_basic_test.cc"],
    deps = [
        ":logging",
        "//core",
        "//core:file",
        "//testing",
    ],
)

cc_test(
    name = "logging_check_test",
    srcs = ["logging_check_test.cc"],
    deps = [
        ":logging",
        "//testing",
    ],
)

cc_test(
    name = "logging_modifiers_test",
    srcs = ["logging_modifiers_test.cc"],
    deps = [
        ":logging",
        "//core",
        "//core:file",
        "//testing",
    ],
)

cc_test(
    name = "logging_vlog_test",
    srcs = ["logging_vlog_test.cc"],
    deps = [
        ":logging",
        "//core",
        "//core:file",
        "//testing",
    ],
)

cc_library(
    name = "logging_internal",
    srcs = ["logging_internal.cc"],
    hdrs = ["logging_internal.h"],
    deps = [":time"],
)

cc_test(
    name = "logging_internal_test",
    srcs = ["logging_internal_test.cc"],
    deps = [
        ":logging_internal",
        ":time",
        "//testing",
    ],
)

cc_library(
    name = "log_queue",
    srcs = [],
    hdrs = ["log_queue.h"],
    deps = [
        ":logging_internal",
        ":mpsc_queue",
    ],
)

cc_test(
    name = "log_queue_test",
    srcs = ["log_queue_test.cc"],
    deps = [
        ":log_queue",
        ":thread",
        "//testing",
    ],
)

cc_library(
    name = "log_queue_map",
    srcs = ["log_queue_map.cc"],
    hdrs = ["log_queue_map.h"],
    deps = [":cpu"],
)

cc_test(
    name = "log_queue_map_test",
    srcs = ["log_queue_map_test.cc"],
    deps = [
        ":log_queue_map",
        "//testing",
    ],
)

cc_library(
    name = "log_writer_thread",
    srcs = ["log_writer_thread.cc"],
    hdrs = ["log_writer_thread.h"],
    deps = [
        ":cpu",
        ":log_queue",
        ":log_queue_map",
        ":logging_internal",
        ":mutex",
        ":notification",
        ":process",
        ":sleep",
        ":thread",
    ],
)

cc_test(
    name = "log_writer_thread_test",
    srcs = ["log_writer_thread_test.cc"],
    deps = [
        ":log_writer_thread",
        "//core",
        "//core:file",
        "//testing",
    ],
)

cc_library(
    name = "mpsc_queue",
    hdrs = ["mpsc_queue.h"],
    deps = [],
)

cc_test(
    name = "mpsc_queue_test",
    srcs = ["mpsc_queue_test.cc"],
    deps = [
        ":mpsc_queue",
        ":sleep",
        ":thread",
        ":time",
        "//testing",
    ],
)

cc_library(
    name = "mutex",
    srcs = ["mutex.cc"],
    hdrs = ["mutex.h"],
    visibility = ["//..."],
    deps = [
        ":thread_annotations",
        ":time",
    ],
)

cc_test(
    name = "mutex_test",
    srcs = ["mutex_test.cc"],
    deps = [
        ":mutex",
        "//core",
        "//testing",
    ],
)

cc_library(
    name = "notification",
    srcs = ["notification.cc"],
    hdrs = ["notification.h"],
    visibility = ["//..."],
    deps = [":time"],
)

cc_test(
    name = "notification_test",
    srcs = ["notification_test.cc"],
    deps = [
        ":notification",
        "//core",
        "//testing",
    ],
)

cc_library(
    name = "numa",
    srcs = ["numa.cc"],
    hdrs = ["numa.h"],
    linker_flags = ["-lnuma"],
    deps = [":cpu"],
)

cc_test(
    name = "numa_test",
    srcs = ["numa_test.cc"],
    deps = [
        ":cpu",
        ":numa",
        "//testing",
    ],
)

cc_library(
    name = "process",
    srcs = ["process.cc"],
    hdrs = ["process.h"],
    visibility = ["//..."],
    deps = [
        ":async_safe",
        ":flags",
        ":raw_syscalls",
        ":stacktrace",
        ":thread",
        ":thread_registry",
    ],
)

cc_library(
    name = "rawlog",
    srcs = ["rawlog.cc"],
    hdrs = ["rawlog.h"],
    visibility = [
        "//base/...",
        "//core/...",
    ],
)

cc_test(
    name = "rawlog_test",
    srcs = ["rawlog_test.cc"],
    deps = [
        ":rawlog",
        "//testing",
    ],
)

cc_library(
    name = "raw_syscalls",
    srcs = ["raw_syscalls.cc"],
    hdrs = ["raw_syscalls.h"],
)

cc_test(
    name = "raw_syscalls_test",
    srcs = ["raw_syscalls_test.cc"],
    deps = [
        ":raw_syscalls",
        "//testing",
    ],
)

cc_binary(
    name = "segfaulter",
    srcs = ["segfaulter.cc"],
    deps = [
        ":process",
        ":thread",
    ],
)

cc_library(
    name = "sleep",
    srcs = ["sleep.cc"],
    hdrs = ["sleep.h"],
    visibility = [
        "//base/...",
        "//core",
    ],
    deps = [":time"],
)

cc_test(
    name = "sleep_test",
    srcs = ["sleep_test.cc"],
    deps = [
        ":sleep",
        ":time",
        "//testing",
    ],
)

cc_library(
    name = "source_location",
    hdrs = ["source_location.h"],
    deps = [],
)

cc_test(
    name = "source_location_test",
    srcs = ["source_location_test.cc"],
    deps = [
        ":source_location",
        "//testing",
    ],
)

cc_library(
    name = "stacktrace",
    srcs = ["stacktrace.cc"],
    hdrs = ["stacktrace.h"],
    deps = [
        ":async_safe",
        ":raw_syscalls",
        ":thread_registry",
    ],
)

cc_test(
    name = "stacktrace_test",
    srcs = ["stacktrace_test.cc"],
    deps = [
        ":stacktrace",
        "//testing",
    ],
)

cc_library(
    name = "thread",
    srcs = ["thread.cc"],
    hdrs = ["thread.h"],
    visibility = ["//..."],
    deps = [":thread_registry"],
)

cc_test(
    name = "thread_test",
    srcs = ["thread_test.cc"],
    deps = [
        ":thread",
        "//core",
        "//testing",
    ],
)

cc_library(
    name = "thread_annotations",
    srcs = [],
    hdrs = ["thread_annotations.h"],
    visibility = ["//..."],
    deps = [],
)

cc_library(
    name = "thread_registry",
    srcs = ["thread_registry.cc"],
    hdrs = ["thread_registry.h"],
    deps = [
        ":async_safe",
        ":mutex",
    ],
)

cc_test(
    name = "thread_registry_test",
    srcs = ["thread_registry_test.cc"],
    deps = [
        ":thread_registry",
        "//testing",
    ],
)

cc_library(
    name = "time",
    srcs = ["time.cc"],
    hdrs = ["time.h"],
    visibility = [
        "//base/...",
        "//core",
    ],
    deps = [":rawlog"],
)

cc_test(
    name = "time_test",
    srcs = ["time_test.cc"],
    deps = [
        ":time",
        "//testing",
    ],
)

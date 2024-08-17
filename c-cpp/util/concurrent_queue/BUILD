
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "concurrent_queue",
    includes = [".."],
    srcs = glob([
        "*.h",
        "*.cc",
    ]),
    copts = [
        "-Werror",
        "-Wall"
        # "-Wno-sign-compare",
        # "-Wno-error=format"
    ],
    deps = [
        "//util/memory_order:memory_order",
        "//util/bitops:bitops",
    ],
)

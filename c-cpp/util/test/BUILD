
package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "spsc_test",
    includes = [".."],
    srcs = glob([
        "spsc_test.cc",
    ]),
    copts = [
        "-Werror",
        "-Wall"
        # "-Wno-sign-compare",
        # "-Wno-error=format"
    ],
    deps = [
        "//util/concurrent_queue:concurrent_queue",
        "//util/util:util",
    ],
)

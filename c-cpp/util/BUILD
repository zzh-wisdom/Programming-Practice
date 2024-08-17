
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "test",
    # includes = ["."],
    srcs = glob([
        "test/*.h",
        "test/*.cc",
    ]),
    copts = [
        "-Werror",
        "-Wall"
        # "-Wno-sign-compare",
        # "-Wno-error=format"
    ],
    deps = [
        "//util:concurrent_queue:concurrent_queue",
        "//util:util:util",
    ],
)

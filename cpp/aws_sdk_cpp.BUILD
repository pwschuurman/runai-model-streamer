load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
    name = "all_srcs",
    srcs = glob(
        ["**"],  # Include all files by default
        exclude = [
            # Exclude everything under any directory named "test/tests"
            # There are some file names with unicode characters that cmake fails on.
            "**/test/**",
            "**/tests/**",
        ],
    ),
    visibility = ["//visibility:public"],
)

cmake(
    name = "s3-crt",
    build_args = [
        "-j4",
    ],
    cache_entries = {
        "BUILD_ONLY": "s3-crt",
        "FORCE_SHARED_CRT": "OFF",
        "BUILD_SHARED_LIBS": "OFF",
        "ENABLE_TESTING": "OFF",
        "AUTORUN_UNIT_TESTS": "OFF",
    },
    lib_source = ":all_srcs",
    out_static_libs = ["libaws-cpp-sdk-s3-crt.a"],
    deps = [
        "@boringssl//:crypto",
        "@boringssl//:ssl",
        "@curl",
        "@zlib",
    ],
    visibility = ["//visibility:public"],
)

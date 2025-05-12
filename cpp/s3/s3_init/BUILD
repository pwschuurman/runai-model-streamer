load("//:rules.bzl", "runai_cc_auto_library", "runai_cc_test")

runai_cc_auto_library(
    name = "s3_init",
    deps = [
        "@aws_sdk_cpp//:s3-crt",
        "//utils/env",
        "//utils/logging",
    ],
)

runai_cc_test(
    name = "s3_init_test",
    srcs = ["s3_init_test.cc"],
    deps = [":s3_init"],
)

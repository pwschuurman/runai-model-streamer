load("//:rules.bzl", "runai_cc_auto_library")

runai_cc_auto_library(
    name = "client",
    deps = [
        "@google_cloud_cpp//:storage_grpc",
        "//gcs/client_configuration",
        "//common/response_code",
        "//common/response",
        "//common/responder",
        "//common/range",
        "//common/s3_credentials",
        "//common/exception",
        "//utils/env",
        "//utils/fd",
    ],
)

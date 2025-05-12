load("//:rules.bzl", "runai_cc_auto_library", "runai_portable_so")

runai_cc_auto_library(
    name = "gcs",
    deps = [
        "//gcs/client_mgr",
    ],
)

runai_portable_so(
    name = "libstreamers3.so",
    srcs = [
        "gcs.h",
        "gcs.cc",
    ],
    deps = [
        "//gcs/client_mgr",
    ],
    ldscript = ":gcs.ldscript",
)

load(":toolchain_config.bzl", "toolchain_config")

platform(
    name = "aarch64",
    constraint_values = [
        "@platforms//cpu:aarch64",
        "@platforms//os:linux",
    ],
)

platform(
    name = "x86_64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
)

# defined in toolchain/cc_toolchain_config.bzl
toolchain_config(name = "aarch64_toolchain_config", os = "linux-gnu", arch = "aarch64", use_cross = False)
toolchain_config(name = "x86_64_toolchain_config", os = "linux-gnu", arch = "x86_64")

filegroup(name = "empty")

cc_toolchain(
   name = "x86_64_toolchain",
   toolchain_identifier = "x86_64-toolchain",
   toolchain_config = ":x86_64_toolchain_config",
   all_files = ":empty",
   compiler_files = ":empty",
   dwp_files = ":empty",
   linker_files = ":empty",
   objcopy_files = ":empty",
   strip_files = ":empty",
   supports_param_files = 0,
)

cc_toolchain(
   name = "aarch64_toolchain",
   toolchain_identifier = "aarch64-toolchain",
   toolchain_config = ":aarch64_toolchain_config",
   all_files = ":empty",
   compiler_files = ":empty",
   dwp_files = ":empty",
   linker_files = ":empty",
   objcopy_files = ":empty",
   strip_files = ":empty",
   supports_param_files = False,
)

toolchain(
    name = "aarch64_linux_toolchain",
    exec_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
    target_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:aarch64",
    ],
    toolchain = ":aarch64_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)

toolchain(
    name = "x86_64_linux_toolchain",
    exec_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
    target_compatible_with = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
    toolchain = ":x86_64_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)
load("@rules_cc//cc:cc_toolchain_config_lib.bzl", "tool_path")  # buildifier: disable=deprecated-function
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")

def __tool_path(toolchain_name, tool_name):
    return "/usr/bin/" + toolchain_name + "-" + tool_name

def runai_crosstool_tools(toolchain_name):
    return {
        "ld":           __tool_path(toolchain_name, "ld"),
        "gcc":          __tool_path(toolchain_name, "gcc"),
        "g++":          __tool_path(toolchain_name, "g++"),
        "ar":           __tool_path(toolchain_name, "ar"),
        "cpp":          __tool_path(toolchain_name, "cpp"),
        "gcov":         __tool_path(toolchain_name, "gcov"),
        "nm":           __tool_path(toolchain_name, "nm"),
        "objdump":      __tool_path(toolchain_name, "objdump"),
        "strip":        __tool_path(toolchain_name, "strip"),
        "cc1plus":      __tool_path(toolchain_name, "cc1plus")
    }

def _builtin_include_directories(toolchain_name, gcc_version):
    return [
        "/usr/include/c++/%d" % gcc_version,
        "/usr/include/%s/c++/%d" % (toolchain_name, gcc_version),
        "/usr/include/c++/%d/backward" % gcc_version,
        "/usr/lib/gcc/%s/%d/include" % (toolchain_name, gcc_version),
        "/usr/local/include",
        "/usr/lib/gcc/%s/%d/include-fixed" % (toolchain_name, gcc_version),
    ]

def _cross_include_directories(toolchain_name, gcc_version):
    return [
        "/usr/lib/gcc-cross/%s/%d/include" % (toolchain_name, gcc_version),
        "/usr/%s/include" % toolchain_name,
    ]

def _get_include_directories(ctx, toolchain_name, use_cross):
    gcc_tool = runai_crosstool_tools(toolchain_name)["gcc"]
    #gcc_version_file = ctx.actions.declare_file("gcc_version.txt")
    #ctx.actions.run_shell(
    #    outputs=[gcc_version_file],
    #    command="%s -dumpversion | cut -f1 -d. > $1" % gcc_tool,
    #)
    #gcc_version = ctx.files.read(gcc_version_file)
    gcc_version = 14

    return _cross_include_directories(toolchain_name, gcc_version) #if ctx.attr.use_cross else _builtin_include_directories(toolchain_name, gcc_version) + [
    #    "/usr/include/%s" % toolchain_name,
    #    "/usr/include"
    #]


def _impl(ctx):
    toolchain_name = ctx.attr.arch + "-" + ctx.attr.os
    use_cross = ctx.attr.use_cross
    tool_paths = [tool_path(name = k, path = v) for k, v in runai_crosstool_tools(toolchain_name).items()]

    # Documented at
    # https://docs.bazel.build/versions/main/skylark/lib/cc_common.html#create_cc_toolchain_config_info.
    #
    # create_cc_toolchain_config_info is the public interface for registering
    # C++ toolchain behavior.
    return cc_common.create_cc_toolchain_config_info(
        ctx = ctx,
        toolchain_identifier = toolchain_name + "-toolchain",
        host_system_name = "local",
        target_system_name = "local",
        target_cpu = ctx.attr.arch,
        target_libc = "unknown",
        compiler = "gcc",
        abi_version = "unknown",
        abi_libc_version = "unknown",
        tool_paths = tool_paths,
        cxx_builtin_include_directories = _get_include_directories(ctx, toolchain_name, use_cross)
    )

toolchain_config = rule(
    implementation = _impl,
    # You can alternatively define attributes here that make it possible to
    # instantiate different cc_toolchain_config targets with different behavior.
    attrs = {
        "os": attr.string(
            mandatory = True,
            doc = "The operating system (eg: linux)",
        ),
        "arch": attr.string(
            mandatory = True,
            doc = "The architecture (eg: x86_64 / aarch64)",
        ),
        "use_cross": attr.bool(
            mandatory = False,
            default = True,
            doc = "Uses crosstool paths if True",
        ),
    },
    provides = [CcToolchainConfigInfo],
)

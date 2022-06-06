package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tag_base",
    hdrs = [
        "tag_base.h",
    ],
    deps = [
        "//src/common/proto:grpc_service",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_gperftools_gperftools//:gperftools",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@glog",
        "@jemalloc",
    ],
)

cc_library(
    name = "client_reader_writer",
    hdrs = [
        "client_rpc_reader.h",
        "client_rpc_writer.h",
    ],
    deps = [
        ":tag_base",
    ],
)

cc_library(
    name = "server_reader_writer",
    hdrs = [
        "server_rpc_reader.h",
        "server_rpc_writer.h",
    ],
    deps = [
        ":tag_base",
    ],
)

cc_library(
    name = "client_impl",
    hdrs = [
        "client_impl.h",
    ],
    deps = [
        ":tag_base",
    ],
)

cc_library(
    name = "server_impl",
    hdrs = [
        "server_impl.h",
    ],
    copts = ["-std=c++14"],
    defines = [
        "BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION=1",
        "BOOST_THREAD_PROVIDES_EXECUTORS",
        "BOOST_BIND_GLOBAL_PLACEHOLDERS",
        "BOOST_THREAD_USES_MOVE",
    ],
    linkopts = [
        "-lpthread",
    ],
    deps = [
        ":tag_base",
        "@boost//:thread",
    ],
)

cc_library(
    name = "client_rpc_tag",
    hdrs = [
        "client_rpc_tag.h",
    ],
    deps = [
        ":client_impl",
        ":client_reader_writer",
        ":tag_base",
    ],
)

cc_library(
    name = "server_rpc_tag",
    hdrs = [
        "server_rpc_tag.h",
    ],
    deps = [
        ":server_impl",
        ":server_reader_writer",
        ":tag_base",
    ],
)

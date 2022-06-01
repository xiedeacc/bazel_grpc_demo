package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "grpc_async_state_stream_server",
    srcs = ["grpc_async_state_stream_server.cc"],
    data = [
        "//data:exported_data",
    ],
    linkstatic = 1,
    deps = [
        ":server_impl",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_gperftools_gperftools//:gperftools",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@glog",
        "@jemalloc",
    ],
)

cc_library(
    name = "tag_info",
    hdrs = ["tag_info.h"],
    deps = [
    ],
)

cc_library(
    name = "server_impl",
    srcs = ["server_impl.cc"],
    hdrs = ["server_impl.h"],
    deps = [
        ":tag_info",
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
        "//src/grpc_server/grpc_async_state_stream_server/handler:grpc_handler",
        "//src/grpc_server/grpc_async_state_stream_server/job:grpc_job",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_gperftools_gperftools//:gperftools",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@glog",
    ],
)

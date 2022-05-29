package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "route_guide_client",
    srcs = ["route_guide_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
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

cc_binary(
    name = "route_guide_callback_client",
    srcs = ["route_guide_callback_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
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

cc_binary(
    name = "route_guide_async_client",
    srcs = ["route_guide_async_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
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

cc_binary(
    name = "route_guide_async_stream_client",
    srcs = ["route_guide_async_stream_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
        "//src/grpc_client/async_stream:route_guide_client",
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

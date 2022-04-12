package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "route_guide_client",
    srcs = ["route_guide_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/util:helper",
        "@com_github_gflags_gflags//:gflags",
        "@glog",
        "@com_github_gperftools_gperftools//:gperftools",
        "@jemalloc",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "route_guide_callback_client",
    srcs = ["route_guide_callback_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/util:helper",
        "@com_github_gflags_gflags//:gflags",
        "@glog",
        "@com_github_gperftools_gperftools//:gperftools",
        "@jemalloc",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "route_guide_async_client",
    srcs = ["route_guide_async_client.cc"],
    linkstatic = 1,
    deps = [
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/util:helper",
        "@com_github_gflags_gflags//:gflags",
        "@glog",
        "@com_github_gperftools_gperftools//:gperftools",
        "@jemalloc",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)


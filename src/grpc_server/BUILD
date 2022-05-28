package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "grpc_server_demo",
    srcs = ["main.cc"],
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
        "//src/grpc_server/handler:grpc_handler",
        "//src/grpc_server/job:grpc_job",
        "//src/grpc_server/proto:error_code_cc_proto",
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/util:helper",
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

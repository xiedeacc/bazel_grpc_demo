package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "grpc_demo",
    srcs = ["main.cc"],
    linkstatic = 1,
    deps = [
        ":server_impl",
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

cc_library(
    name = "tag_info",
    hdrs = ["tag_info.h"],
    deps = [
    ],
)

cc_library(
    name = "server_impl",
    hdrs = ["server_impl.h"],
    srcs = ["server_impl.cc"],
    deps = [
        ":tag_info",
        "//src/proto:grpc_service",
        "//src/proto:error_code_cc_proto",
        "//src/job:grpc_job",
        "//src/util:helper",
        "//src/handler:grpc_handler",
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


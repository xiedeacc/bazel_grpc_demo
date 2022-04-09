package(default_visibility = ["//visibility:public"])
load("//tools:cpplint.bzl", "cpplint")

cc_binary(
    name = "tbox",
    srcs = ["main.cc"],
    linkstatic = 1,
    deps = [
        "//src/grpc_service:grpc_service_impl",
        "//src/grpc_service:grpc_async_server",
        "//src/common:common",
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

cpplint()

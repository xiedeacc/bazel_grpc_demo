load("//tools:cpplint.bzl", "cpplint")
load("//tools:build.bzl", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "grpc_handler",
    hdrs = [
        "base_handler.h",
        "bidirectional_streaming_handler.h",
        "client_streaming_handler.h",
        "server_streaming_handler.h",
        "unary_handler.h",
    ],
    deps = [
        "//src/proto:grpc_service",
        "//src/job:grpc_base_job",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)

cpplint()

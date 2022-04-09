
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "grpc_job",
    hdrs = [
        "bidirectional_streaming_job.h",
        "client_streaming_job.h",
        "server_streaming_job.h",
        "unary_job.h",
    ],
    srcs = ["static_member_init.cc"],
    deps = [
        ":grpc_base_job",
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/handler:grpc_handler",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)

cc_library(
    name = "grpc_base_job",
    hdrs = [
        "base_job.h",
    ],
    deps = [
        "//src/grpc_server/proto:grpc_service",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)


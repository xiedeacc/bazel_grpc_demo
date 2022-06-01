package(default_visibility = ["//visibility:public"])

cc_library(
    name = "grpc_job",
    srcs = ["static_member_init.cc"],
    hdrs = [
        "bidirectional_streaming_job.h",
        "client_streaming_job.h",
        "server_streaming_job.h",
        "unary_job.h",
    ],
    deps = [
        ":grpc_base_job",
        "//src/common/proto:grpc_service",
        "//src/grpc_server/grpc_async_state_stream_server/handler:grpc_handler",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_googletest//:gtest",
    ],
)

cc_library(
    name = "grpc_base_job",
    hdrs = [
        "base_job.h",
    ],
    deps = [
        "//src/common/proto:grpc_service",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@com_google_googletest//:gtest",
        "@glog",
    ],
)

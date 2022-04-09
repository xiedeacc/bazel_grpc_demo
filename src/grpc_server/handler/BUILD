
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
        "//src/grpc_server/proto:grpc_service",
        "//src/grpc_server/job:grpc_base_job",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)


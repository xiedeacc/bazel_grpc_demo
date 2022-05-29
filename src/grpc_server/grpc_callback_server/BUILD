package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "grpc_callback_server",
    srcs = ["grpc_callback_server.cc"],
    deps = [
        "//src/common/proto:grpc_service",
        "//src/common/util:helper",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)

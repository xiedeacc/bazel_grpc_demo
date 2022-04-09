load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")
package(default_visibility = ["//visibility:public"])

cc_grpc_library(
    name = "grpc_service",
    srcs = [":grpc_service_proto"],
    deps = [
      ":grpc_service_cc_proto",

    ],
    grpc_only = True,
)

cc_proto_library(
    name = "grpc_service_cc_proto",
    deps = [":grpc_service_proto"],
)

proto_library(
    name = "grpc_service_proto",
    srcs = ["grpc_service.proto"],
    deps = [
    ]
)

cc_proto_library(
    name = "error_code_cc_proto",
    deps = [":error_code"],
)

proto_library(
    name = "error_code",
    srcs = ["error_code.proto"],
    deps = [
    ]
)

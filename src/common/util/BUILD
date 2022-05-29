package(default_visibility = ["//visibility:public"])

cc_library(
    name = "helper",
    srcs = [
        "helper.cc",
    ],
    hdrs = [
        "helper.h",
    ],
    deps = [
        "//src/common/proto:grpc_service",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
        "@glog",
    ],
)

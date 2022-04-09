package(default_visibility = ["//visibility:public"])

cc_library(
    name = "helper",
    hdrs = [
        "helper.h",
    ],
    srcs = [
         "helper.cc",
    ],
    deps = [
        "//src/proto:grpc_service",
        "@com_github_gflags_gflags//:gflags",
        "@com_google_googletest//:gtest",
        "@glog",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc++_reflection",
    ],
)



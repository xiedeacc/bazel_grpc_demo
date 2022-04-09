package(default_visibility = ["//visibility:public"])
load("//tools:cpplint.bzl", "cpplint")
load("//tools:build.bzl", "cc_test")

cc_library(
  name = "job_create_factory",
  hdrs = ["job_create_factory.h"],
  srcs = ["job_create_factory.cc"],
  deps = [
    ":grpc_service_base",
    "//src/grpc_service/job:grpc_job",
    "@com_github_grpc_grpc//:grpc++",
    "@com_github_grpc_grpc//:grpc++_reflection",
  ],
)

cc_library(
  name = "grpc_service_impl",
  srcs = ["grpc_service_impl.cc"],
  hdrs = ["grpc_service_impl.h"],
  defines = [
    "BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION=1",
    "BOOST_THREAD_PROVIDES_EXECUTORS",
    "BOOST_BIND_GLOBAL_PLACEHOLDERS",
    "BOOST_THREAD_USES_MOVE",
  ],
  deps = [
    ":job_create_factory",
    ":grpc_service_base",
    "//src/grpc_service/proto:grpc_service",
    "//src/grpc_service/job:grpc_job",
    "//src/grpc_service/proto:error_code_cc_proto",
    "@com_github_gflags_gflags//:gflags",
    "@com_google_googletest//:gtest",
    "//src/common:common",
    "//src/context:server_run_info",
    "//src/context:grpc_context",
    "@com_github_grpc_grpc//:grpc++",
    "@com_github_grpc_grpc//:grpc++_reflection",
    "@boost//:algorithm",
    "@boost//:thread",
    "//src/util:common_util",
    "//src/util:config_util",
    "//src/util:blocking_list",
    "//src/util:garbage_collector",
  ],
)

cc_binary(
    name = "grpc_demo",
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

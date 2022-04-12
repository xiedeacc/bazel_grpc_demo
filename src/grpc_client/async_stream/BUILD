
package(default_visibility = ["//visibility:public"])

cc_library(
    name = "client_base",
    hdrs = [
      "client_base.h",
    ],
    deps = [
        "//src/grpc_client/async_stream/grpc_framework:client_impl",
        "//src/grpc_server/proto:grpc_service",
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

cc_library(
    name = "route_guide_call",
    hdrs = [
      "route_guide_call.h",
    ],
    srcs = ["route_guide_call.cc"],
    deps = [
        ":client_base",
        "//src/grpc_client/async_stream/grpc_framework:client_rpc_tag",
        "//src/grpc_server/proto:grpc_service",
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

cc_library(
    name = "route_guide_client",
    hdrs = [
      "route_guide_client.h",
    ],
    srcs = [
      "route_guide_client.cc",
    ],

    deps = [
        ":client_base",
        ":route_guide_call",
        "//src/grpc_client/async_stream/grpc_framework:client_impl",
        "//src/grpc_server/proto:grpc_service",
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


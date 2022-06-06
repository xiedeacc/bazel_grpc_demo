package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "grpc_async_server",
    srcs = ["grpc_async_server.cc"],
    linkstatic = 1,
    deps = [
        ":route_guide_server_impl",
    ],
)

cc_library(
    name = "route_guide_server_base",
    hdrs = [
        "route_guide_server_base.h",
    ],
    deps = [
        "//src/common/grpc_framework:server_impl",
        "//src/common/proto:grpc_service",
    ],
)

cc_library(
    name = "route_chat_call",
    srcs = ["route_chat_call.cc"],
    hdrs = [
        "route_chat_call.h",
    ],
    deps = [
        ":route_guide_server_base",
        "//src/common/grpc_framework:server_rpc_tag",
        "//src/common/proto:grpc_service",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_gperftools_gperftools//:gperftools",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@glog",
        "@jemalloc",
    ],
)

cc_library(
    name = "route_guide_server_impl",
    srcs = [
        "route_guide_server_impl.cc",
    ],
    hdrs = [
        "route_guide_server_impl.h",
    ],
    deps = [
        ":route_chat_call",
        "//src/common/util:helper",
    ],
)

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tag_base",
    hdrs = [
      "tag_base.h",
    ],
    deps = [
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
    name = "reader_writer",
    hdrs = [
      "rpc_reader.h",
      "rpc_writer.h",
    ],
    deps = [
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
    name = "msg_queue",
    hdrs = [
      "msg_queue.h",
    ],
    deps = [
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
    name = "client_impl",
    hdrs = [
      "client_impl.h",
    ],
    deps = [
        ":tag_base",
        "//src/grpc_server/proto:grpc_service",
        "@com_github_grpc_grpc//:grpc++",
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
    name = "client_rpc_tag",
    hdrs = [
      "client_rpc_tag.h",
    ],
    deps = [
        ":client_impl",
        ":tag_base",
        ":reader_writer",
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


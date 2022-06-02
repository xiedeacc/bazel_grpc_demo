package(default_visibility = ["//visibility:public"])

cc_library(
    name = "tag_base",
    hdrs = [
        "tag_base.h",
    ],
    deps = [
    ],
)

cc_library(
    name = "reader_writer",
    hdrs = [
        "rpc_reader.h",
        "rpc_writer.h",
    ],
    deps = [
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
    name = "client_impl",
    hdrs = [
        "client_impl.h",
    ],
    deps = [
        ":tag_base",
        "//src/common/proto:grpc_service",
        "@boost//:algorithm",
        "@boost//:thread",
        "@com_github_gflags_gflags//:gflags",
        "@com_github_gperftools_gperftools//:gperftools",
        "@com_github_grpc_grpc//:grpc++",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
        "@glog",
        "@jemalloc",
    ],
)

cc_library(
    name = "client_rpc_tag",
    hdrs = [
        "client_rpc_tag.h",
    ],
    deps = [
        ":client_impl",
        ":reader_writer",
        ":tag_base",
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

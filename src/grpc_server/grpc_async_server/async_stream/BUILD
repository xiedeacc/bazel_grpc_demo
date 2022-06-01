#package(default_visibility = ["//visibility:public"])

#cc_library(
#name = "client_base",
#hdrs = [
#"client_base.h",
#],
#deps = [
#"//src/common/grpc_framework:client_impl",
#"//src/common/proto:grpc_service",
#"@boost//:algorithm",
#"@boost//:thread",
#"@com_github_gflags_gflags//:gflags",
#"@com_github_gperftools_gperftools//:gperftools",
#"@com_google_absl//absl/strings",
#"@com_google_absl//absl/time",
#"@glog",
#"@jemalloc",
#],
#)

#cc_library(
#name = "route_guide_call",
#srcs = ["route_guide_call.cc"],
#hdrs = [
#"route_guide_call.h",
#],
#deps = [
#":client_base",
#"//src/common/grpc_framework:client_rpc_tag",
#"//src/common/proto:grpc_service",
#"@boost//:algorithm",
#"@boost//:thread",
#"@com_github_gflags_gflags//:gflags",
#"@com_github_gperftools_gperftools//:gperftools",
#"@com_google_absl//absl/strings",
#"@com_google_absl//absl/time",
#"@glog",
#"@jemalloc",
#],
#)

#cc_library(
#name = "route_guide_client",
#srcs = [
#"route_guide_client.cc",
#],
#hdrs = [
#"route_guide_client.h",
#],
#deps = [
#":client_base",
#":route_guide_call",
#"//src/common/grpc_framework:client_impl",
#"//src/common/proto:grpc_service",
#"@boost//:algorithm",
#"@boost//:thread",
#"@com_github_gflags_gflags//:gflags",
#"@com_github_gperftools_gperftools//:gperftools",
#"@com_google_absl//absl/strings",
#"@com_google_absl//absl/time",
#"@glog",
#"@jemalloc",
#],
#)

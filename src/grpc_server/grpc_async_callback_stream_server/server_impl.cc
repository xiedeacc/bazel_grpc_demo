#include "src/grpc_server/grpc_async_callback_stream_server/server_impl.h"
namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {

std::vector<grpc_demo::common::proto::Feature> ServerImpl::mFeatureList;
std::unordered_map<
    grpc_demo::grpc_server::grpc_async_callback_stream_server::job::BaseJob *,
    ServerImpl::RecordRouteState>
    ServerImpl::mRecordRouteMap;

} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#include "src/grpc_server/server_impl.h"
namespace grpc_demo {
namespace grpc_server {

std::vector<grpc_demo::grpc_server::Feature> ServerImpl::mFeatureList;
std::unordered_map<grpc_demo::grpc_server::job::BaseJob *,
                   ServerImpl::RecordRouteState>
    ServerImpl::mRecordRouteMap;

} // namespace grpc_server
} // namespace grpc_demo

#include "src/server_impl.h"
namespace grpc_demo {

std::vector<grpc_demo::Feature> ServerImpl::mFeatureList;
std::unordered_map<grpc_demo::job::BaseJob *, ServerImpl::RecordRouteState>
    ServerImpl::mRecordRouteMap;

} // namespace grpc_demo

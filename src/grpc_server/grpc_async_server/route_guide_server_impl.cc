//
// Created by zjf on 2018/2/13.
//

#include "src/grpc_server/grpc_async_server/route_guide_server_impl.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_server/grpc_async_server/route_chat_call.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_server {
using grpc_demo::common::proto::RouteNote;

void ServerImpl::OnRun(
    std::unique_ptr<grpc::ServerCompletionQueue> &request_queue,
    std::unique_ptr<grpc::ServerCompletionQueue> &response_queue) {
  new RouteChatCall(this, request_queue, response_queue);
};

void ServerImpl::OnExit() {}

void ServerImpl::OnRouteChatRead(
    const grpc_demo::common::proto::RouteNote *request,
    grpc_demo::common::proto::RouteNote *response) {
  LOG(INFO) << "Read message " << request->message() << " at "
            << request->location().latitude() << ", "
            << request->location().longitude();
  response->CopyFrom(*request);
}

void ServerImpl::OnRouteChatWrite(int write_id) {
  // LOG(INFO) << "write_id: " << write_id;
}
} // namespace grpc_async_server
} // namespace grpc_server
} // namespace grpc_demo

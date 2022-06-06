//
// Created by zjf on 2018/3/12.
//

#include "src/grpc_server/grpc_async_server/route_chat_call.h"
#include "src/common/grpc_framework/server_rpc_tag.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_server {

using grpc_demo::common::proto::Point;
using grpc_demo::common::proto::RouteNote;

RouteChatCall::RouteChatCall(
    RouteGuideServerBase *server,
    std::unique_ptr<grpc::ServerCompletionQueue> &request_queue,
    std::unique_ptr<grpc::ServerCompletionQueue> &response_queue)
    : server_(server), request_queue_(request_queue),
      response_queue_(response_queue) {

  server_->service_.RequestRouteChat(&server_context_, &responder_,
                                     request_queue_.get(),
                                     response_queue_.get(), this);

  reader_ = std::unique_ptr<SuperTag::ReaderType>(
      new SuperTag::ReaderType(this, responder_));

  writer_ = std::unique_ptr<SuperTag::WriterType>(
      new SuperTag::WriterType(this, responder_));

  writer_->Start();
}

void RouteChatCall::OnRead(void *message) {
  // LOG(INFO) << "OnRead";
  RouteNote *response = writer_->NewResponse();
  server_->OnRouteChatRead((grpc_demo::common::proto::RouteNote *)message,
                           response);
  writer_->Write(response);
}

void RouteChatCall::OnReadError() { writer_->Write(nullptr); }

void RouteChatCall::OnWrite(int write_id) {
  server_->OnRouteChatWrite(write_id);
}

void RouteChatCall::Process() {
  if (status == grpc_demo::common::grpc_framework::ServerRPCStatus::CREATE) {
    LOG(INFO) << "RouteChatCall CREATE, address: " << this;
    reader_->Read();
  } else if (status ==
             grpc_demo::common::grpc_framework::ServerRPCStatus::FINISH) {
    LOG(INFO) << "RouteChatCall FINISH, now delete address: " << this;
    delete this;
  }
}

} // namespace grpc_async_server
} // namespace grpc_server
} // namespace grpc_demo

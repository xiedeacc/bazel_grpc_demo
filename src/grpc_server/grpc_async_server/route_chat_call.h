#ifndef ROUTE_CHAT_CALL_H
#define ROUTE_CHAT_CALL_H

#include "src/common/grpc_framework/server_rpc_tag.h"
#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_server/grpc_async_server/route_guide_server_base.h"
#include <list>
#include <string>

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_server {

class RouteChatCall
    : public grpc_demo::common::grpc_framework::ServerBiStreamRpcTag<
          grpc_demo::common::proto::RouteNote,
          grpc_demo::common::proto::RouteNote> {
public:
  typedef grpc_demo::common::grpc_framework::ServerBiStreamRpcTag<
      grpc_demo::common::proto::RouteNote, grpc_demo::common::proto::RouteNote>
      SuperTag;

  RouteChatCall(RouteGuideServerBase *server,
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
    new RouteChatCall(server, &request_queue, &response_queue);
  }

  virtual void OnRead(void *message) override {
    // LOG(INFO) << "OnRead";
    grpc_demo::common::proto::RouteNote *response = writer_->NewResponse();
    server_->OnRouteChatRead((grpc_demo::common::proto::RouteNote *)message,
                             response);
    writer_->Write(response);
  }

  virtual void OnWrite(int write_id) override {
    server_->OnRouteChatWrite(write_id);
  };

  virtual void OnReadError() override { writer_->Write(nullptr); }

  virtual void Process() override {
    if (status == grpc_demo::common::grpc_framework::ServerRPCStatus::CREATE) {
      LOG(INFO) << "RouteChatCall CREATE, address: " << this;
      reader_->Read();
    } else if (status ==
               grpc_demo::common::grpc_framework::ServerRPCStatus::FINISH) {
      LOG(INFO) << "RouteChatCall FINISH, now delete address: " << this;
      delete this;
    }
  }

  virtual std::string Name() { return "RouteChatCall"; }

private:
  RouteGuideServerBase *server_;
  const std::unique_ptr<grpc::ServerCompletionQueue> &request_queue_;
  const std::unique_ptr<grpc::ServerCompletionQueue> &response_queue_;
};

} // namespace grpc_async_server
} // namespace grpc_server
} // namespace grpc_demo
#endif // ROUTE_CHAT_CALL_H

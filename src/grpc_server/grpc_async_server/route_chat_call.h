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
                std::unique_ptr<grpc::ServerCompletionQueue> &response_queue);

  virtual void OnRead(void *) override;
  virtual void OnWrite(int) override;

  virtual void OnReadError() override;

  virtual void Process() override;

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

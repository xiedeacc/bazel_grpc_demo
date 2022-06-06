#ifndef ROUTE_GUIDE_CALL_H
#define ROUTE_GUIDE_CALL_H

#include "src/common/grpc_framework/client_rpc_tag.h"
#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_client/async_stream/client_base.h"
#include <list>
#include <string>

namespace grpc_demo {
namespace grpc_client {
namespace async_stream {
class RouteGuideCall
    : public grpc_demo::common::grpc_framework::ClientBiStreamRpcTag<
          grpc_demo::common::proto::RouteNote,
          grpc_demo::common::proto::RouteNote> {
public:
  typedef grpc_demo::common::grpc_framework::ClientBiStreamRpcTag<
      grpc_demo::common::proto::RouteNote, grpc_demo::common::proto::RouteNote>
      SuperTag;
  RouteGuideCall(ClientBase *client);

  virtual void OnRead(void *) override;
  virtual void OnReadError() override;

  virtual void OnWrite(int) override;

  virtual void Process() override;

private:
  ClientBase *client;
  std::list<grpc_demo::common::proto::RouteNote> notes_;
};

} // namespace async_stream
} // namespace grpc_client
} // namespace grpc_demo
#endif // ROUTE_GUIDE_CALL_H

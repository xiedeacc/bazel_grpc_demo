#ifndef ROUTE_GUIDE_CALL_H
#define ROUTE_GUIDE_CALL_H

#include "src/grpc_client/async_stream/client_base.h"
#include "src/grpc_client/async_stream/grpc_framework/client_rpc_tag.h"
#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"
#include <list>
#include <string>

class RouteGuideCall : public grpc_framework::ClientBiStreamRpcTag<
                           grpc_demo::grpc_server::RouteNote,
                           grpc_demo::grpc_server::RouteNote> {
public:
  typedef grpc_framework::ClientBiStreamRpcTag<
      grpc_demo::grpc_server::RouteNote, grpc_demo::grpc_server::RouteNote>
      SuperTag;
  RouteGuideCall(ClientBase *client);

  virtual void OnRead(void *) override;
  virtual void OnWrite(int) override;

  virtual void Process() override;
  virtual void Finish() override;

private:
  ClientBase *client;
  std::list<grpc_demo::grpc_server::RouteNote> notes_;
};

#endif // ROUTE_GUIDE_CALL_H

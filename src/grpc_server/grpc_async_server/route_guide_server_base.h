
//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_SERVER_BASE_H
#define ROUTE_GUIDE_SERVER_BASE_H

#include <grpcpp/grpcpp.h>
#include <thread>

#include "src/common/grpc_framework/server_impl.h"
#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_server {
class RouteGuideServerBase
    : public grpc_demo::common::grpc_framework::ServerImpl<
          grpc_demo::common::proto::RouteGuide> {
public:
  virtual void
  OnRouteChatRead(const grpc_demo::common::proto::RouteNote *request,
                  grpc_demo::common::proto::RouteNote *response) = 0;

  virtual void OnRouteChatWrite(int write_id) = 0;

protected:
};
} // namespace grpc_async_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // ROUTE_GUIDE_SERVER_BASE_H

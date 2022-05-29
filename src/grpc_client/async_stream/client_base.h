
//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_CLIENT_BASE_H
#define ROUTE_GUIDE_CLIENT_BASE_H

#include "src/common/grpc_framework/client_impl.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc/grpc.h>

#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"

#include <thread>
namespace grpc_demo {
namespace grpc_client {
namespace async_stream {
class ClientBase : public grpc_demo::common::grpc_framework::ClientImpl<
                       grpc_demo::common::proto::RouteGuide> {
public:
  virtual void OnRouteChatRead(void *message) = 0;
  virtual void OnRouteChatWrite(void *message) = 0;

protected:
};
} // namespace async_stream
} // namespace grpc_client
} // namespace grpc_demo

#endif // ROUTE_GUIDE_CLIENT_BASE_H

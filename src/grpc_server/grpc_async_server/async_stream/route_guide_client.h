//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_CLIENT_H
#define ROUTE_GUIDE_CLIENT_H

#include "src/common/grpc_framework/client_impl.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc/grpc.h>

#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_client/async_stream/client_base.h"
#include <thread>

namespace grpc_demo {
namespace grpc_client {
namespace async_stream {
class RouteGuideClient : public ClientBase {
public:
  RouteGuideClient();

  virtual void OnRun() override;
  virtual void OnExit() override;

  virtual void OnRouteChatRead(void *message);
  virtual void OnRouteChatWrite(void *message);

protected:
};

} // namespace async_stream
} // namespace grpc_client
} // namespace grpc_demo
#endif // ROUTE_GUIDE_CLIENT_H

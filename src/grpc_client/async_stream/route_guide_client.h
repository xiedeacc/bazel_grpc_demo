//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_CLIENT_H
#define ROUTE_GUIDE_CLIENT_H

#include "src/grpc_client/async_stream/grpc_framework/client_impl.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc/grpc.h>

#include "src/grpc_client/async_stream/client_base.h"
#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"
#include <thread>

class RouteGuideClient : public ClientBase {
public:
  RouteGuideClient();

  virtual void OnRun() override;
  virtual void OnExit() override;

  virtual void OnRouteChatRead(void *message);
  virtual void OnRouteChatWrite(void *message);

protected:
};

#endif // ROUTE_GUIDE_CLIENT_H

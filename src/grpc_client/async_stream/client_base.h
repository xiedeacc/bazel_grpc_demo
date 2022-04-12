
//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_CLIENT_BASE_H
#define ROUTE_GUIDE_CLIENT_BASE_H

#include "src/grpc_client/async_stream/grpc_framework/client_impl.h"
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc/grpc.h>

#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"

#include <thread>

class ClientBase
    : public grpc_framework::ClientImpl<grpc_demo::grpc_server::RouteGuide> {
public:
  virtual void OnRouteChatRead(void *message) = 0;
  virtual void OnRouteChatWrite(void *message) = 0;

protected:
};

#endif // ROUTE_GUIDE_CLIENT_BASE_H

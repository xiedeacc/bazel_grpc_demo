//
// Created by zjf on 2018/2/13.
//

#include "src/grpc_client/async_stream/route_guide_client.h"
#include "src/grpc_client/async_stream/route_guide_call.h"
#include "src/grpc_server/proto/grpc_service.pb.h"
#include <grpc++/create_channel.h>

using grpc_demo::grpc_server::RouteNote;

RouteGuideClient::RouteGuideClient() {}

void RouteGuideClient::OnRun() { new RouteGuideCall(this); }

void RouteGuideClient::OnExit() {}

void RouteGuideClient::OnRouteChatRead(void *message) {
  const RouteNote *note = static_cast<RouteNote *>(message);
  std::cout << "Read message " << note->message() << " at "
            << note->location().latitude() << ", "
            << note->location().longitude() << std::endl;
}

void RouteGuideClient::OnRouteChatWrite(void *message) {
  const RouteNote *note = static_cast<RouteNote *>(message);
  std::cout << "Sending message " << note->message() << " at "
            << note->location().latitude() << ", "
            << note->location().longitude() << std::endl;
}

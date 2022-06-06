//
// Created by zjf on 2018/3/12.
//

#include "src/grpc_client/async_stream/route_chat_call.h"
#include "src/common/grpc_framework/client_rpc_tag.h"

namespace grpc_demo {
namespace grpc_client {
namespace async_stream {
using grpc_demo::common::proto::Point;
using grpc_demo::common::proto::RouteNote;

Point MakePoint(long latitude, long longitude) {
  Point p;
  p.set_latitude(latitude);
  p.set_longitude(longitude);
  return p;
}

RouteNote MakeRouteNote(const std::string &message, long latitude,
                        long longitude) {
  RouteNote n;
  n.set_message(message);
  n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return n;
}

RouteGuideCall::RouteGuideCall(ClientBase *client) : client(client) {
  stream = client->stub()->AsyncRouteChat(&context, client->cq(), this);
  reader_ = std::unique_ptr<SuperTag::ReaderType>(
      new SuperTag::ReaderType(this, *(stream.get())));
  writer_ = std::unique_ptr<SuperTag::WriterType>(
      new SuperTag::WriterType(this, *(stream.get())));
  writer_->Start();
  notes_.push_back(MakeRouteNote("First message", 0, 0));
  notes_.push_back(MakeRouteNote("Second message", 0, 1));
  notes_.push_back(MakeRouteNote("Third message", 1, 0));
  notes_.push_back(MakeRouteNote("Fourth message", 1, 1));
}

void RouteGuideCall::OnRead(void *message) { client->OnRouteChatRead(message); }

void RouteGuideCall::OnReadError() {
  LOG(INFO) << "OnReadError";
  Finish();
  client->Exit();
}

void RouteGuideCall::OnWrite(int write_id) {
  // client->OnRouteChatWrite(message);
}

void RouteGuideCall::Process() {
  if (status == grpc_demo::common::grpc_framework::ClientRPCStatus::CREATE) {
    LOG(INFO) << "RouteGuideCall CREATE";
    status = grpc_demo::common::grpc_framework::ClientRPCStatus::READ;
    const auto p = writer_->Write(notes_.begin(), notes_.end());
    if (p.first == -1 && p.second == -1) {
      LOG(INFO) << "write error";
    }
    reader_->Read();
  } else if (status ==
             grpc_demo::common::grpc_framework::ClientRPCStatus::FINISH) {
    LOG(INFO) << "RouteGuideCall FINISH";
    delete this;
  }
}
} // namespace async_stream
} // namespace grpc_client
} // namespace grpc_demo

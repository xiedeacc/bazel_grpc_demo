//
// Created by zjf on 2018/3/12.
//

#include "src/grpc_client/async_stream/route_guide_call.h"

using grpc_demo::grpc_server::Point;
using grpc_demo::grpc_server::RouteNote;

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
  writer_ = std::unique_ptr<super::WriterType>(
      new super::WriterType(this, *(stream.get())));
  writer_->Start();
  // client->AddTag(this);
  std::vector<RouteNote> notes_;
  notes_.push_back(MakeRouteNote("First message", 0, 0));
  notes_.push_back(MakeRouteNote("Second message", 0, 1));
  notes_.push_back(MakeRouteNote("Third message", 1, 0));
  notes_.push_back(MakeRouteNote("Fourth message", 0, 0));
  for (const auto &note : notes_) {
    writer_->Write(note);
  }
}

void RouteGuideCall::OnRead(void *message) { client->OnRouteChatRead(message); }

void RouteGuideCall::OnWrite(int write_id) {
  // client->OnRouteChatWrite(message);
}

void RouteGuideCall::Process() {
  if (status == grpc_framework::ClientRPCStatus::CREATE) {
    LOG(INFO) << "RouteGuideCall CREATE";
    status = grpc_framework::ClientRPCStatus::READ;
    reader_->Read();
  } else if (status == grpc_framework::ClientRPCStatus::FINISH) {
    LOG(INFO) << "RouteGuideCall FINISH";
    client->RemoveTag({this, reader_.get()});
    delete this;
  }
}

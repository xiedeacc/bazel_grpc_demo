/*
 *
 * Copyright 2021 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"
#include "src/grpc_server/util/helper.h"
#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc_demo::grpc_server::Feature;
using grpc_demo::grpc_server::Point;
using grpc_demo::grpc_server::Rectangle;
using grpc_demo::grpc_server::RouteGuide;
using grpc_demo::grpc_server::RouteNote;
using grpc_demo::grpc_server::RouteSummary;

Point MakePoint(long latitude, long longitude) {
  Point p;
  p.set_latitude(latitude);
  p.set_longitude(longitude);
  return p;
}

Feature MakeFeature(const std::string &name, long latitude, long longitude) {
  Feature f;
  f.set_name(name);
  f.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return f;
}

RouteNote MakeRouteNote(const std::string &message, long latitude,
                        long longitude) {
  RouteNote n;
  n.set_message(message);
  n.mutable_location()->CopyFrom(MakePoint(latitude, longitude));
  return n;
}

class RouteGuideClient {
  enum class Type {
    READ = 1,
    WRITE = 2,
    CONNECT = 3,
    WRITES_DONE = 4,
    FINISH = 5
  };

public:
  RouteGuideClient(std::shared_ptr<Channel> channel, const std::string &db)
      : stub_(grpc_demo::grpc_server::RouteGuide::NewStub(channel)) {
    grpc_thread_.reset(
        new std::thread(std::bind(&RouteGuideClient::GrpcThread, this)));
    // context_.AsyncNotifyWhenDone(reinterpret_cast<void *>(Type::FINISH));
  }

  ~RouteGuideClient() {
    std::cout << "Shutting down client...." << std::endl;
    cq_.Shutdown();
    grpc_thread_->join();
  }

  void RouteChat() {
    notes_.clear();
    notes_.push_back(MakeRouteNote("First message", 0, 0));
    notes_.push_back(MakeRouteNote("Second message", 0, 1));
    notes_.push_back(MakeRouteNote("Third message", 1, 0));
    notes_.push_back(MakeRouteNote("Fourth message", 0, 0));

    stream_ = stub_->AsyncRouteChat(&context_, &cq_,
                                    reinterpret_cast<void *>(Type::CONNECT));
    for (const auto &note : notes_) {
      Write(note);
    }
  }

private:
  void Write(const RouteNote &note) {
    static int index = 0;
    if (index == 4) {
      stream_->WritesDone(reinterpret_cast<void *>(Type::WRITES_DONE));
      return;
    }

    std::cout << "Sending message " << note.message() << " at "
              << note.location().latitude() << ", "
              << note.location().longitude() << std::endl;

    stream_->Write(note, reinterpret_cast<void *>(Type::WRITE));
    ++index;
  }

  void Read() {
    std::cout << "Got message " << response_.message() << " at "
              << response_.location().latitude() << ", "
              << response_.location().longitude() << std::endl;

    stream_->Read(&response_, reinterpret_cast<void *>(Type::WRITE));
  }

  void GrpcThread() {
    while (true) {
      void *got_tag;
      bool ok = false;
      if (!cq_.Next(&got_tag, &ok)) {
        std::cerr << "Client stream closed. Quitting" << std::endl;
        break;
      }

      switch (static_cast<Type>(reinterpret_cast<long>(got_tag))) {
      case Type::READ:
        std::cout << "Read a new message." << std::endl;
        break;
      case Type::WRITE:
        std::cout << "Sending message (async)." << std::endl;
        Read();
        break;
      case Type::CONNECT:
        std::cout << "Server connected." << std::endl;
        break;
      case Type::WRITES_DONE:
        std::cout << "Write done." << std::endl;
        Read();
        break;
      case Type::FINISH:
        std::cout << "Client finish; status = " << (ok ? "ok" : "cancelled")
                  << std::endl;
        // context_.TryCancel();
        break;
      default:
        std::cerr << "Unexpected tag " << got_tag << std::endl;
        GPR_ASSERT(false);
      }
    }
  }

  grpc::ClientContext context_;
  grpc::CompletionQueue cq_;
  std::vector<RouteNote> notes_;
  std::unique_ptr<grpc_demo::grpc_server::RouteGuide::Stub> stub_;
  std::unique_ptr<grpc::ClientAsyncReaderWriter<RouteNote, RouteNote>> stream_;
  RouteNote response_;
  std::unique_ptr<std::thread> grpc_thread_;
  grpc::Status status_;
};

int main(int argc, char **argv) {
  std::string db = grpc_demo::grpc_server::util::GetDbFileContent(argc, argv);
  RouteGuideClient guide(
      grpc::CreateChannel("localhost:50051",
                          grpc::InsecureChannelCredentials()),
      db);
  std::cout << "-------------- RouteChat --------------" << std::endl;
  guide.RouteChat();
  return 0;
}
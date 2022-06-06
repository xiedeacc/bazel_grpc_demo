#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>

#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/common/util/helper.h"
#include "src/grpc_client/async_stream/route_chat_call.h"
#include "src/grpc_client/async_stream/route_guide_client.h"
#include <grpc/grpc.h>
#include <grpcpp/alarm.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc_demo::common::proto::Feature;
using grpc_demo::common::proto::Point;
using grpc_demo::common::proto::Rectangle;
using grpc_demo::common::proto::RouteGuide;
using grpc_demo::common::proto::RouteNote;
using grpc_demo::common::proto::RouteSummary;

int main(int argc, char **argv) {
  grpc_demo::grpc_client::async_stream::RouteGuideClient guide;
  std::cout << "-------------- RouteChat --------------" << std::endl;
  guide.Run("localhost:50051");
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  return 0;
}

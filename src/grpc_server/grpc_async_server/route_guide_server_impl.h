//
// Created by zjf on 2018/2/13.
//

#ifndef ROUTE_GUIDE_SERVER_H
#define ROUTE_GUIDE_SERVER_H

#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/common/util/helper.h"
#include "src/grpc_server/grpc_async_server/route_guide_server_base.h"
#include <grpcpp/grpcpp.h>
#include <thread>

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_server {
struct RecordRouteState {
  int pointCount;
  int featureCount;
  float distance;
  grpc_demo::common::proto::Point previous;
  std::chrono::system_clock::time_point startTime;
  RecordRouteState() : pointCount(0), featureCount(0), distance(0.0f) {}
};
class ServerImpl final : public RouteGuideServerBase {
public:
  ServerImpl(const std::string &db_content) {
    grpc_demo::common::util::ParseDb(db_content, &feature_list_);
  };

  virtual void
  OnRun(std::unique_ptr<grpc::ServerCompletionQueue> &request_queue,
        std::unique_ptr<grpc::ServerCompletionQueue> &response_queue) override;

  virtual void OnExit() override;

  virtual void
  OnRouteChatRead(const grpc_demo::common::proto::RouteNote *request,
                  grpc_demo::common::proto::RouteNote *response);
  virtual void OnRouteChatWrite(int write_id);

  std::unordered_map<grpc_demo::common::grpc_framework::TagBase *,
                     RecordRouteState>
      record_route_map_;

  std::vector<grpc_demo::common::proto::Feature> feature_list_;
};

} // namespace grpc_async_server
} // namespace grpc_server
} // namespace grpc_demo
#endif // ROUTE_GUIDE_SERVER_H

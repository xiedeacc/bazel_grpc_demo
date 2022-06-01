/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H
#include <grpcpp/support/status.h>
#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

#include "boost/thread/executors/basic_thread_pool.hpp"
#include "grpc++/grpc++.h"
#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/common/util/helper.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/base_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/bidirectional_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/client_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/server_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/unary_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/base_job.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/bidirectional_streaming_job.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/client_streaming_job.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/server_streaming_job.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/unary_job.h"
#include "src/grpc_server/grpc_async_state_stream_server/tag_info.h"
#include "gtest/gtest_prod.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {

class ServerImpl {
public:
  ServerImpl(const std::string &db_content, std::mutex &incoming_tags_mutex,
             std::list<TagInfo> &incoming_tags)
      : incoming_tags_mutex_(incoming_tags_mutex),
        incoming_tags_(incoming_tags) {
    grpc_demo::common::util::ParseDb(db_content, &mFeatureList);
  }

  ~ServerImpl() {
    mServer->Shutdown();
    // Always shutdown the completion queue after the server.
    mCQ->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&mRouteGuideService);
    mCQ = builder.AddCompletionQueue();
    mServer = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;
    HandleRpcs();
  }

private:
  static void CreateGetFeatureRpc(
      grpc_demo::common::proto::RouteGuide::AsyncService *async_service,
      grpc::ServerCompletionQueue *request_queue,
      grpc::ServerCompletionQueue *response_queue) {
    grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
        UnaryHandler<grpc_demo::common::proto::RouteGuide::AsyncService,
                     grpc_demo::common::proto::Point,
                     grpc_demo::common::proto::Feature>
            rpcHandlers;

    rpcHandlers.CreateJob = CreateGetFeatureRpc;

    rpcHandlers.Done = &ServerImpl::Done;

    rpcHandlers.ProcessIncomingRequest = &ServerImpl::GetFeatureProcessor;

    rpcHandlers.RequestRpc =
        &grpc_demo::common::proto::RouteGuide::AsyncService::RequestGetFeature;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::UnaryJob<
        grpc_demo::common::proto::RouteGuide::AsyncService,
        grpc_demo::common::proto::Point, grpc_demo::common::proto::Feature>(
        async_service, request_queue, response_queue, rpcHandlers);
  }

  static void Done(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *job,
      bool rpc_cancelled) {
    delete job;
  }

  static grpc::Status
  GetFeatureProcessor(grpc::ServerContext *server_context, const void *data,
                      const grpc_demo::common::proto::Point *request,
                      grpc_demo::common::proto::Feature *response) {
    auto point = static_cast<const grpc_demo::common::proto::Point *>(request);

    response->set_name(grpc_demo::common::util::GetFeatureName(
        *point, grpc_demo::grpc_server::grpc_async_state_stream_server::
                    ServerImpl::mFeatureList));
    response->mutable_location()->CopyFrom(*point);
  }

  static void CreateListFeaturesRpc(
      grpc_demo::common::proto::RouteGuide::AsyncService *async_service,
      grpc::ServerCompletionQueue *request_queue,
      grpc::ServerCompletionQueue *response_queue) {
    grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
        ServerStreamingHandler<
            grpc_demo::common::proto::RouteGuide::AsyncService,
            grpc_demo::common::proto::Rectangle,
            grpc_demo::common::proto::Feature>
            rpcHandlers;

    rpcHandlers.CreateJob = CreateListFeaturesRpc;

    rpcHandlers.ProcessIncomingRequest = &ServerImpl::ListFeaturesProcessor;
    rpcHandlers.Done = &ServerImpl::Done;

    rpcHandlers.RequestRpc = &grpc_demo::common::proto::RouteGuide::
                                 AsyncService::RequestListFeatures;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::
        ServerStreamingJob<grpc_demo::common::proto::RouteGuide::AsyncService,
                           grpc_demo::common::proto::Rectangle,
                           grpc_demo::common::proto::Feature>(
            async_service, request_queue, response_queue, rpcHandlers);
  }

  static grpc::Status
  ListFeaturesProcessor(grpc::ServerContext *server_context, const void *data,
                        const grpc_demo::common::proto::Rectangle *request,
                        grpc_demo::common::proto::Feature *response) {
    auto rectangle =
        static_cast<const grpc_demo::common::proto::Rectangle *>(request);

    auto lo = rectangle->lo();
    auto hi = rectangle->hi();
    long left = (std::min)(lo.longitude(), hi.longitude());
    long right = (std::max)(lo.longitude(), hi.longitude());
    long top = (std::max)(lo.latitude(), hi.latitude());
    long bottom = (std::min)(lo.latitude(), hi.latitude());
    for (auto f : grpc_demo::grpc_server::grpc_async_state_stream_server::
             ServerImpl::mFeatureList) {
      if (f.location().longitude() >= left &&
          f.location().longitude() <= right &&
          f.location().latitude() >= bottom && f.location().latitude() <= top) {
        response->CopyFrom(f);
      }
    }
  }

  static void CreateRecordRouteRpc(
      grpc_demo::common::proto::RouteGuide::AsyncService *async_service,
      grpc::ServerCompletionQueue *request_queue,
      grpc::ServerCompletionQueue *response_queue) {
    grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
        ClientStreamingHandler<
            grpc_demo::common::proto::RouteGuide::AsyncService,
            grpc_demo::common::proto::Point,
            grpc_demo::common::proto::RouteSummary>
            rpcHandlers;

    rpcHandlers.CreateJob = CreateRecordRouteRpc;

    rpcHandlers.ProcessIncomingRequest = &ServerImpl::RecordRouteProcessor;
    rpcHandlers.Done = Done;

    rpcHandlers.RequestRpc =
        &grpc_demo::common::proto::RouteGuide::AsyncService::RequestRecordRoute;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::
        ClientStreamingJob<grpc_demo::common::proto::RouteGuide::AsyncService,
                           grpc_demo::common::proto::Point,
                           grpc_demo::common::proto::RouteSummary>(
            async_service, request_queue, response_queue, rpcHandlers);
  }

  struct RecordRouteState {
    int pointCount;
    int featureCount;
    float distance;
    grpc_demo::common::proto::Point previous;
    std::chrono::system_clock::time_point startTime;
    RecordRouteState() : pointCount(0), featureCount(0), distance(0.0f) {}
  };

  static grpc::Status
  RecordRouteProcessor(grpc::ServerContext *server_context, const void *data,
                       const grpc_demo::common::proto::Point *request,
                       grpc_demo::common::proto::RouteSummary *response) {
    auto point = static_cast<const grpc_demo::common::proto::Point *>(request);

    RecordRouteState &state = grpc_demo::grpc_server::
        grpc_async_state_stream_server::ServerImpl::mRecordRouteMap[(
            grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob
                *)data];

    if (point) {
      if (state.pointCount == 0)
        state.startTime = std::chrono::system_clock::now();

      state.pointCount++;
      if (!grpc_demo::common::util::GetFeatureName(
               *point, grpc_demo::grpc_server::grpc_async_state_stream_server::
                           ServerImpl::mFeatureList)
               .empty()) {
        state.featureCount++;
      }
      if (state.pointCount != 1) {
        state.distance +=
            grpc_demo::common::util::GetDistance(state.previous, *point);
      }
      state.previous = *point;

    } else {
      std::chrono::system_clock::time_point endTime =
          std::chrono::system_clock::now();

      grpc_demo::common::proto::RouteSummary &summary =
          *((grpc_demo::common::proto::RouteSummary *)response);
      summary.set_point_count(state.pointCount);
      summary.set_feature_count(state.featureCount);
      summary.set_distance(static_cast<long>(state.distance));
      auto secs = std::chrono::duration_cast<std::chrono::seconds>(
          endTime - state.startTime);
      summary.set_elapsed_time(secs.count());

      grpc_demo::grpc_server::grpc_async_state_stream_server::ServerImpl::
          mRecordRouteMap.erase(
              (grpc_demo::grpc_server::grpc_async_state_stream_server::job::
                   BaseJob *)data);
    }
  }

  static void CreateRouteChatRpc(
      grpc_demo::common::proto::RouteGuide::AsyncService *async_service,
      grpc::ServerCompletionQueue *request_queue,
      grpc::ServerCompletionQueue *response_queue) {
    grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
        BidirectionalStreamingHandler<
            grpc_demo::common::proto::RouteGuide::AsyncService,
            grpc_demo::common::proto::RouteNote,
            grpc_demo::common::proto::RouteNote>
            rpcHandlers;

    rpcHandlers.CreateJob = CreateRouteChatRpc;

    rpcHandlers.ProcessIncomingRequest = RouteChatProcessor;
    rpcHandlers.Done = Done;

    rpcHandlers.RequestRpc =
        &grpc_demo::common::proto::RouteGuide::AsyncService::RequestRouteChat;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::
        BidirectionalStreamingJob<
            grpc_demo::common::proto::RouteGuide::AsyncService,
            grpc_demo::common::proto::RouteNote,
            grpc_demo::common::proto::RouteNote>(async_service, request_queue,
                                                 response_queue, rpcHandlers);
  }

  static grpc::Status
  RouteChatProcessor(grpc::ServerContext *server_context, const void *data,
                     const grpc_demo::common::proto::RouteNote *request,
                     grpc_demo::common::proto::RouteNote *response) {
    auto note =
        static_cast<const grpc_demo::common::proto::RouteNote *>(request);
    // Simply echo the note back.
    if (note) {
      ((grpc_demo::common::proto::RouteNote *)response)->CopyFrom(*note);
    } else {
    }
  }

  void HandleRpcs() {
    CreateGetFeatureRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateListFeaturesRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateRecordRouteRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateRouteChatRpc(&mRouteGuideService, mCQ.get(), mCQ.get());

    TagInfo tagInfo;
    while (true) {
      GPR_ASSERT(mCQ->Next((void **)&tagInfo.tagProcessor,
                           &tagInfo.ok)); // GRPC_TODO - Handle returned value

      incoming_tags_mutex_.lock();
      incoming_tags_.push_back(tagInfo);
      incoming_tags_mutex_.unlock();
    }
  }

  std::list<TagInfo> &incoming_tags_;
  std::mutex &incoming_tags_mutex_;
  std::unique_ptr<grpc::ServerCompletionQueue> mCQ;
  grpc_demo::common::proto::RouteGuide::AsyncService mRouteGuideService;
  static std::unordered_map<
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *,
      RecordRouteState>
      mRecordRouteMap;

  static std::vector<grpc_demo::common::proto::Feature> mFeatureList;
  std::unique_ptr<grpc::Server> mServer;
};

} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // SERVER_IMPL_H

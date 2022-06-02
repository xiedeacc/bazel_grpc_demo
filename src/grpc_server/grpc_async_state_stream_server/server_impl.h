/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H
#include "job/base_job.h"
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
#include "gtest/gtest_prod.h"
#include <thread>

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {

class ServerImpl {
public:
  ServerImpl(const std::string &db_content) {
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

    rpcHandlers.Done = GetFeatureDone;

    rpcHandlers.ProcessIncomingRequest = &ServerImpl::GetFeatureProcessor;

    rpcHandlers.RequestRpc =
        &grpc_demo::common::proto::RouteGuide::AsyncService::RequestGetFeature;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::UnaryJob<
        grpc_demo::common::proto::RouteGuide::AsyncService,
        grpc_demo::common::proto::Point, grpc_demo::common::proto::Feature>(
        async_service, request_queue, response_queue, rpcHandlers);
  }

  static void
  GetFeatureProcessor(grpc::ServerContext *server_context, const void *data,
                      const grpc_demo::common::proto::Point *request,
                      grpc_demo::common::proto::Feature *response) {
    auto point = static_cast<const grpc_demo::common::proto::Point *>(request);

    response->set_name(grpc_demo::common::util::GetFeatureName(
        *point, grpc_demo::grpc_server::grpc_async_state_stream_server::
                    ServerImpl::mFeatureList));
    response->mutable_location()->CopyFrom(*point);
  }

  static void GetFeatureDone(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *job,
      bool rpc_cancelled) {
    LOG(INFO) << "Done called! rpc_cancelled: "
              << (rpc_cancelled ? "true" : "false");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    rpcHandlers.Done = ListFeaturesDone;

    rpcHandlers.RequestRpc = &grpc_demo::common::proto::RouteGuide::
                                 AsyncService::RequestListFeatures;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::
        ServerStreamingJob<grpc_demo::common::proto::RouteGuide::AsyncService,
                           grpc_demo::common::proto::Rectangle,
                           grpc_demo::common::proto::Feature>(
            async_service, request_queue, response_queue, rpcHandlers);
  }

  static void
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

  static void ListFeaturesDone(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *job,
      bool rpc_cancelled) {
    LOG(INFO) << "Done called! rpc_cancelled: "
              << (rpc_cancelled ? "true" : "false");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    rpcHandlers.Done = RecordRouteDone;

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

  static void
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

  static void RecordRouteDone(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *job,
      bool rpc_cancelled) {
    LOG(INFO) << "Done called! rpc_cancelled: "
              << (rpc_cancelled ? "true" : "false");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    rpcHandlers.Done = RouteChatDone;

    rpcHandlers.RequestRpc =
        &grpc_demo::common::proto::RouteGuide::AsyncService::RequestRouteChat;

    new grpc_demo::grpc_server::grpc_async_state_stream_server::job::
        BidirectionalStreamingJob<
            grpc_demo::common::proto::RouteGuide::AsyncService,
            grpc_demo::common::proto::RouteNote,
            grpc_demo::common::proto::RouteNote>(async_service, request_queue,
                                                 response_queue, rpcHandlers);
  }

  static void
  RouteChatProcessor(grpc::ServerContext *server_context, const void *data,
                     const grpc_demo::common::proto::RouteNote *request,
                     grpc_demo::common::proto::RouteNote *response) {
    auto note =
        static_cast<const grpc_demo::common::proto::RouteNote *>(request);
    if (note) {
      response->CopyFrom(*note);
    }
  }

  static void RouteChatDone(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *job,
      bool rpc_cancelled) {
    LOG(INFO) << "Done called! rpc_cancelled: "
              << (rpc_cancelled ? "true" : "false");
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  void HandleRpcs() {
    CreateGetFeatureRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateListFeaturesRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateRecordRouteRpc(&mRouteGuideService, mCQ.get(), mCQ.get());
    CreateRouteChatRpc(&mRouteGuideService, mCQ.get(), mCQ.get());

    while (true) {
      void *tag;
      bool ok;
      bool ret = mCQ->Next(&tag, &ok);
      if (tag == nullptr) {
        LOG(ERROR) << "this should never happen...";
        continue;
      }
      LOG(INFO) << "job address: " << tag
                << ", ok: " << (ok ? "true" : "false");
      if (!tag) {
        LOG(INFO) << "tag already deleted!";
        continue;
      }
      ((grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *)
           tag)
          ->Proceed(ok);
    }
  }

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

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef HANDLER_BASE_HANDLER_H
#define HANDLER_BASE_HANDLER_H
#include "src/common/proto/grpc_service.grpc.pb.h"
#pragma once

#include <grpcpp/completion_queue.h>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include <functional>

#include "google/protobuf/service.h"
#include "grpc++/grpc++.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/base_job.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace handler {

template <typename ServiceType, typename RequestType, typename ResponseType>
struct BaseHandler {
public:
  using CreateJobFun =
      void (*)(grpc_demo::common::proto::RouteGuide::AsyncService *,
               grpc::ServerCompletionQueue *, grpc::ServerCompletionQueue *);

  using ProcessIncomingRequestFun =
      void (*)(grpc::ServerContext *server_context, const void *,
               const RequestType *, ResponseType *);

  using DoneFun = void (*)(
      grpc_demo::grpc_server::grpc_async_state_stream_server::job::BaseJob *,
      bool);

  CreateJobFun CreateJob;

  ProcessIncomingRequestFun ProcessIncomingRequest;

  DoneFun Done;
};

} // namespace handler
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // HANDLER_BASE_HANDLER_H

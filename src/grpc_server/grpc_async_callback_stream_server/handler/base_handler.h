/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef HANDLER_BASE_HANDLER_H
#define HANDLER_BASE_HANDLER_H
#pragma once

#include <grpcpp/completion_queue.h>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include <functional>

#include "google/protobuf/service.h"
#include "grpc++/grpc++.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/base_job.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {
namespace handler {

using CreateRpc = std::function<void()>;

using ProcessIncomingRequest = std::function<void(
    grpc_demo::grpc_server::grpc_async_callback_stream_server::job::BaseJob &,
    const google::protobuf::Message *)>;

using Done = std::function<void(
    grpc_demo::grpc_server::grpc_async_callback_stream_server::job::BaseJob &,
    bool)>;

template <typename ServiceType, typename RequestType, typename ResponseType>
struct BaseHandlers {
public:
  CreateRpc createRpc;

  ProcessIncomingRequest processIncomingRequest;

  Done done;
};

} // namespace handler
} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // HANDLER_BASE_HANDLER_H

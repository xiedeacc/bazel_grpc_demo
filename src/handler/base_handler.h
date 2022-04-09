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
#include "src/job/base_job.h"

namespace grpc_demo {
namespace handler {

using CreateRpc =
    std::function<void(grpc::Service *, grpc::ServerCompletionQueue *)>;

using ProcessIncomingRequest = std::function<void(
    grpc_demo::job::BaseJob &, const google::protobuf::Message *)>;

using Done = std::function<void(grpc_demo::job::BaseJob &, bool)>;

template <typename ServiceType, typename RequestType, typename ResponseType>
struct BaseHandlers {
public:
  CreateRpc createRpc;

  ProcessIncomingRequest processIncomingRequest;

  Done done;
};

} // namespace handler
} // namespace grpc_demo

#endif // HANDLER_BASE_HANDLER_H

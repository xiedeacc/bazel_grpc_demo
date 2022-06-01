/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef HANDLER_SERVER_STREAM_HANDLER_H
#define HANDLER_SERVER_STREAM_HANDLER_H
#pragma once

#include <grpcpp/completion_queue.h>

#include "src/grpc_server/grpc_async_callback_stream_server/handler/base_handler.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {
namespace handler {

template <typename ServiceType, typename RequestType, typename ResponseType>
struct ServerStreamingHandlers
    : public BaseHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder = grpc::ServerAsyncWriter<ResponseType>;

  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, RequestType *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  RequestRpc requestRpc;
};

} // namespace handler
} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // HANDLER_SERVER_STREAM_HANDLER_H

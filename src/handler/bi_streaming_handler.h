/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef HANDLER_BI_STREAM_HANDLER_H
#define HANDLER_BI_STREAM_HANDLER_H
#pragma once

#include <grpcpp/completion_queue.h>

#include "src/grpc_service//handler/base_handler.h"

namespace grpc_demo {
namespace handler {

template <typename ServiceType, typename RequestType, typename ResponseType>
struct BidirectionalStreamingHandlers
    : public BaseHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder =
      grpc::ServerAsyncReaderWriter<ResponseType, RequestType>;
  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  RequestRpc requestRpc;
};

} // namespace handler
} // namespace grpc_demo

#endif // HANDLER_BI_STREAM_HANDLER_H

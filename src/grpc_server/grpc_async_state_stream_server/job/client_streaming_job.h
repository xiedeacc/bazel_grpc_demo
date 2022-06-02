
/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef grpc_server_RPC_CLIENT_STREAMING_RPC_H
#define grpc_server_RPC_CLIENT_STREAMING_RPC_H
#include <grpcpp/completion_queue.h>
#pragma once

#include "src/grpc_server/grpc_async_state_stream_server/handler/base_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/bidirectional_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/client_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/server_streaming_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/unary_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/base_job.h"
namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace job {

template <typename ServiceType, typename RequestType, typename ResponseType>
class ClientStreamingJob : public BaseJob {
  using ThisRpcTypeHandler =
      grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
          ClientStreamingHandler<ServiceType, RequestType, ResponseType>;

public:
  ClientStreamingJob(ServiceType *async_service,
                     grpc::ServerCompletionQueue *request_queue,
                     grpc::ServerCompletionQueue *response_queue,
                     ThisRpcTypeHandler handler)
      : BaseJob(request_queue, response_queue), async_service_(async_service),
        responder_(&server_context_), handler_(handler),
        client_streaming_done_(false) {
    ++client_streaming_rpc_counter;
    Proceed(true);
  }

protected:
  void RequestRpc(bool ok) {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
    status_ = READ;
    handler_.RequestRpc(async_service_, &server_context_, &responder_,
                        request_queue_, response_queue_, this);
  }

  void Init(bool ok) {
    LOG(INFO) << "Init";
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
  }

  // READ
  void ReadRequest(bool ok) {
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        status_ = PROCESS;
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        responder_.Read(&request_, this);
      }
    }
  }

  // PROCESS
  void HandleRequest(bool ok) {
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        responder_.Read(&request_, this);
      } else {
        client_streaming_done_ = true;
        handler_.CreateJob(async_service_, request_queue_, response_queue_);
        handler_.ProcessIncomingRequest(&server_context_, this, nullptr,
                                        &response_);
        SendResponse(&response_);
      }
    }
  }

  bool SendResponse(const google::protobuf::Message *response) {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(response_, grpc::Status::OK, this);
    status_ = FINISH;
    return true;
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.FinishWithError(error, this);
    return true;
  }

  void Done() override {
    handler_.Done(this, server_context_.IsCancelled());
    --client_streaming_rpc_counter;
    LOG(INFO) << "Pending Client Streaming Rpcs Count = "
              << client_streaming_rpc_counter;
  }

public:
  static std::atomic<int32_t> client_streaming_rpc_counter;

private:
  ServiceType *async_service_;
  typename ThisRpcTypeHandler::GRPCResponder responder_;
  ThisRpcTypeHandler handler_;
  RequestType request_;
  ResponseType response_;
  bool client_streaming_done_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // grpc_server_RPC_CLIENT_STREAMING_RPC_H

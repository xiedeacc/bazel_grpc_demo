
/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_async_service_RPC_SERVER_STREAMING_RPC_H
#define GRPC_async_service_RPC_SERVER_STREAMING_RPC_H
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
class ServerStreamingJob : public BaseJob {
  using ThisRpcTypeHandler =
      grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
          ServerStreamingHandler<ServiceType, RequestType, ResponseType>;

public:
  ServerStreamingJob(ServiceType *async_service,
                     grpc::ServerCompletionQueue *request_queue,
                     grpc::ServerCompletionQueue *response_queue,
                     ThisRpcTypeHandler handler)
      : BaseJob(request_queue, response_queue), async_service_(async_service),
        responder_(&server_context_), handler_(handler),
        server_streaming_done_(false) {
    ++server_streaming_rpc_counter;
    Proceed(true);
  }

protected:
  // CREATE
  void RequestRpc(bool ok) {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);

    handler_.RequestRpc(async_service_, &server_context_, &request_,
                        &responder_, request_queue_, response_queue_, this);

    status_ = PROCESS;
  }

  void Init(bool ok) {
    LOG(INFO) << "Init";
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
  }
  // PROCESS
  void HandleRequest(bool ok) {
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
        SendResponse(&response_);
      }
    }
  }

  bool SendResponse(const google::protobuf::Message *responseMsg) {
    auto response = static_cast<const ResponseType *>(responseMsg);
    if (response != nullptr) {
      if (!AsyncWriteInProgress()) {
        status_ = WRITE;
        DoSendResponse();
      }
    } else {
      server_streaming_done_ = true;
      if (!AsyncWriteInProgress()) {
        status_ = FINISH;
        DoFinish();
      }
    }
    return true;
  }

  void DoSendResponse() {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_WRITE);
    responder_.Write(response_, this);
  }

  void WriteResponseQueue(bool ok) {
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_WRITE)) {
      if (ok) {
        status_ = WRITE;
        DoSendResponse();
      } else if (server_streaming_done_) {
        status_ = FINISH;
        DoFinish();
      }
    }
  }

  // READ
  void ReadRequest(bool ok) {}

  void DoFinish() {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(grpc::Status::OK, this);
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(error, this);
    return true;
  }

  void Done() override {
    handler_.Done(this, server_context_.IsCancelled());
    --server_streaming_rpc_counter;
    LOG(INFO) << "Pending Server Streaming Rpcs Count = "
              << server_streaming_rpc_counter;
  }

public:
  static std::atomic<int32_t> server_streaming_rpc_counter;

private:
  ServiceType *async_service_;
  typename ThisRpcTypeHandler::GRPCResponder responder_;
  ThisRpcTypeHandler handler_;
  RequestType request_;
  ResponseType response_;
  bool server_streaming_done_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_async_service_RPC_SERVER_STREAMING_RPC_H

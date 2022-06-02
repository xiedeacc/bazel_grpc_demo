/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_async_service_RPC_UNARY_RPC_H
#define GRPC_async_service_RPC_UNARY_RPC_H
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
class UnaryJob : public BaseJob {
  using ThisRpcTypeHandler =
      grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
          UnaryHandler<ServiceType, RequestType, ResponseType>;

public:
  UnaryJob(ServiceType *async_service,
           grpc::ServerCompletionQueue *request_queue,
           grpc::ServerCompletionQueue *response_queue,
           ThisRpcTypeHandler handler)
      : BaseJob(request_queue, response_queue), async_service_(async_service),
        responder_(&server_context_), handler_(handler) {
    ++unary_rpc_counter;
    Proceed(true);
  }

protected:
  // CREATE
  void RequestRpc(bool ok) {
    server_context_.AsyncNotifyWhenDone(this);
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
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        handler_.CreateJob(async_service_, request_queue_, response_queue_);
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
        SendResponse(&response_);
        status_ = FINISH;
      } else {
        // FinishWithError(ok);
      }
    }
  }
  // READ
  void ReadRequest(bool ok) {}

  void WriteResponseQueue(bool ok) {}

  bool SendResponse(const google::protobuf::Message *responseMsg) {
    auto response = static_cast<const ResponseType *>(responseMsg);
    if (response == nullptr) {
      // FinishWithError(ok);
      return false;
    }

    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(response_, grpc::Status::OK, this);
    status_ = FINISH;
    return true;
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.FinishWithError(error, this);
    status_ = FINISH;
    return true;
  }

  void Done() override {
    handler_.Done(this, server_context_.IsCancelled());
    --unary_rpc_counter;
    LOG(INFO) << "Pending Unary Rpcs Count = " << unary_rpc_counter;
  }

public:
  static std::atomic<int32_t> unary_rpc_counter;

private:
  ServiceType *async_service_;
  typename ThisRpcTypeHandler::GRPCResponder responder_;
  ThisRpcTypeHandler handler_;
  RequestType request_;
  ResponseType response_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_async_service_RPC_UNARY_RPC_H

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_async_service_RPC_UNARY_RPC_H
#define GRPC_async_service_RPC_UNARY_RPC_H
#pragma once

#include "src/grpc_server/grpc_async_state_stream_server/handler/base_handler.h"
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
    LOG(INFO) << "pending unary rpcs count = " << unary_rpc_counter;
    server_context_.AsyncNotifyWhenDone(&job_done_handler_);
    Proceed(true);
  }

protected:
  // CREATE
  void RequestRpc(bool ok) {
    status_ = INIT;
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
    handler_.RequestRpc(async_service_, &server_context_, &request_,
                        &responder_, request_queue_, response_queue_, this);
  }

  void Init(bool ok) {
    LOG(INFO) << "Init";
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
        SendResponse(&response_);
      } else {
        SendResponse(nullptr);
      }
    }
  }

  bool SendResponse(const google::protobuf::Message *response_msg) {
    LOG(INFO) << "SendResponse, nullptr: "
              << (!response_msg ? "true" : "false");
    auto response = static_cast<const ResponseType *>(response_msg);
    if (response != nullptr) {
      if (!AsyncWriteInProgress()) {
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
        status_ = FINISH;
        responder_.Finish(response_, grpc::Status::OK, this);
      }
    } else {
      LOG(ERROR) << "this should never happen!";
      FinishWithError(grpc::Status(grpc::StatusCode::INTERNAL,
                                   "this should never happen!"));
    }
    return true;
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.FinishWithError(error, this);
    status_ = FINISH;
    return true;
  }

  void Done() override {
    LOG(INFO) << "Done! async_op_counter: " << async_op_counter_
              << ", job address: " << this;
    if (async_op_counter_ == 0) {
      --unary_rpc_counter;
      LOG(INFO) << "pending unary rpcs count = " << unary_rpc_counter;
      handler_.Done(this, server_context_.IsCancelled());
      delete this;
    }
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

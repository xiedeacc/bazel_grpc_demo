/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef grpc_server_RPC_BI_STREAMING_RPC_H
#define grpc_server_RPC_BI_STREAMING_RPC_H
#pragma once
#include <functional>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/status_code_enum.h>
#include <thread>

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
class BidirectionalStreamingJob : public BaseJob {
  using ThisRpcTypeHandler =
      grpc_demo::grpc_server::grpc_async_state_stream_server::handler::
          BidirectionalStreamingHandler<ServiceType, RequestType, ResponseType>;

public:
  BidirectionalStreamingJob(ServiceType *async_service,
                            grpc::ServerCompletionQueue *request_queue,
                            grpc::ServerCompletionQueue *response_queue,
                            ThisRpcTypeHandler handler)
      : BaseJob(request_queue, response_queue), async_service_(async_service),
        responder_(&server_context_), handler_(handler),
        server_streaming_done_(false), client_streaming_done_(false) {
    ++bi_streaming_rpc_counter;
    LOG(INFO) << "pending bidirectional streaming rpcs count = "
              << bi_streaming_rpc_counter;
    server_context_.AsyncNotifyWhenDone(this);
    Proceed(true);
  }

  virtual ~BidirectionalStreamingJob() override {}

protected:
  // CREATE
  void RequestRpc(bool ok) {
    status_ = INIT;
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
    handler_.RequestRpc(async_service_, &server_context_, &responder_,
                        request_queue_, response_queue_, this);
  }

  void Init(bool ok) {
    LOG(INFO) << "Init";
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        status_ = PROCESS;
        responder_.Read(&request_, this);
      }
    }
  }

  // READ
  void ReadRequest(bool ok) {
    // LOG(INFO) << "ReadRequest, ok: " << (ok ? "true" : "false");
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
    // LOG(INFO) << "HandleRequest, ok: " << (ok ? "true" : "false");
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        LOG(INFO) << "message: " << request_.message()
                  << ", latitude: " << request_.location().latitude()
                  << ", longitude: " << request_.location().longitude();
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
        SendResponse(&response_);
      } else {
        client_streaming_done_ = true;
        // LOG(INFO) << "client_streaming_done_: true, send nullptr";
        SendResponse(nullptr);
      }
    }
  }

  bool SendResponse(const google::protobuf::Message *response_msg) {
    LOG(INFO) << "SendResponse, nullptr: "
              << (!response_msg ? "true" : "false");
    auto response = static_cast<const ResponseType *>(response_msg);
    if (response == nullptr && !client_streaming_done_) {
      LOG(ERROR) << "this should never happen!";
      FinishWithError(grpc::Status(grpc::StatusCode::INTERNAL,
                                   "this should never happen!"));
      return false;
    }

    if (response != nullptr) {
      if (!AsyncWriteInProgress()) {
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_WRITE);
        status_ = READ;
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        responder_.Write(response_, this);
        AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_WRITE);
      }
    } else {
      server_streaming_done_ = true;
      if (!AsyncWriteInProgress()) {
        status_ = FINISH;
        AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        responder_.Finish(grpc::Status::OK, this);
      }
    }
    return true;
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(error, this);
    return true;
  }

  void Done() override {
    LOG(INFO) << "Done! async_op_counter: " << async_op_counter_
              << ", job address: " << this;
    if (async_op_counter_ == 0) {
      --bi_streaming_rpc_counter;
      LOG(INFO) << "pending bidirectional streaming rpcs count = "
                << bi_streaming_rpc_counter;
      handler_.Done(this, server_context_.IsCancelled());
      // handler_.Done(this, true);
      delete this;
    }
  }

public:
  static std::atomic<int32_t> bi_streaming_rpc_counter;

private:
  ServiceType *async_service_;
  typename ThisRpcTypeHandler::GRPCResponder responder_;
  ThisRpcTypeHandler handler_;
  RequestType request_;
  ResponseType response_;
  bool server_streaming_done_;
  bool client_streaming_done_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // grpc_server_RPC_BI_STREAMING_RPC_H

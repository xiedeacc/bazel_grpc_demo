/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef grpc_server_RPC_BI_STREAMING_RPC_H
#define grpc_server_RPC_BI_STREAMING_RPC_H
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
    on_done_ = std::bind(&BaseJob::OnDone, this, std::placeholders::_1);
    Proceed(true);
  }

protected:
  // CREATE
  void RequestRpc(bool ok) {
    server_context_.AsyncNotifyWhenDone(&on_done_);
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);

    handler_.RequestRpc(async_service_, &server_context_, &responder_,
                        request_queue_, response_queue_, this);
    status_ = READ;
  }

private:
  // READ
  void ReadRequest(bool ok) {
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
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

  bool SendResponse(const google::protobuf::Message *response_msg) {
    auto response = static_cast<const ResponseType *>(response_msg);
    if (response == nullptr && !client_streaming_done_) {
      // FinishWithError();
      return false;
    }

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

  void DoFinish() {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(grpc::Status::OK, this);
  }

  bool FinishWithErrorImpl(const grpc::Status &error) override {
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(error, this);
    return true;
  }

  void Done() override { // TODO trigger condition?
    handler_.Done(this, server_context_.IsCancelled());
    --bi_streaming_rpc_counter;
    LOG(INFO) << "Pending Bidirectional Streaming Rpcs Count = "
              << bi_streaming_rpc_counter;
  }

public:
  static std::atomic<int32_t> bi_streaming_rpc_counter;

private:
  std::function<void(bool)> on_done_;
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

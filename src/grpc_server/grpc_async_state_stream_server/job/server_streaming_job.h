
/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_async_service_RPC_SERVER_STREAMING_RPC_H
#define GRPC_async_service_RPC_SERVER_STREAMING_RPC_H
#pragma once

#include "src/grpc_server/grpc_async_state_stream_server/handler/base_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/handler/server_streaming_handler.h"
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
    LOG(INFO) << "pending server streaming rpcs count = "
              << server_streaming_rpc_counter;
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
    LOG(INFO) << "Init, "
              << "ok: " << (ok ? "true" : "false");
    handler_.CreateJob(async_service_, request_queue_, response_queue_);
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        handler_.ProcessIncomingRequest(&server_context_, this, &request_,
                                        &response_);
      }
    }
  }

  bool SendResponse(const google::protobuf::Message *response_msg) {
    LOG(INFO) << "SendResponse, nullptr: "
              << (!response_msg ? "true" : "false");
    auto response = static_cast<const ResponseType *>(response_msg);
    if (response != nullptr) {
      response_list_.push_back(*response);
      grpc_demo::common::proto::Feature *feature =
          (grpc_demo::common::proto::Feature *)response;
      LOG(INFO) << "response_list size: " << response_list_.size();
      LOG(INFO) << "name: " << feature->name()
                << ", latitude: " << feature->location().latitude()
                << ", longitude: " << feature->location().longitude();
      if (!AsyncWriteInProgress()) {
        DoSendResponse();
      }
    } else {
      server_streaming_done_ = true;
      if (!AsyncWriteInProgress()) {
        DoFinish();
      }
    }
    return true;
  }

  void DoSendResponse() {
    status_ = WRITE;
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_WRITE);
    responder_.Write(response_list_.front(), this);
  }

  void WriteNext(bool ok) {
    if (AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_WRITE)) {
      response_list_.pop_front();
      if (ok) {
        if (!response_list_.empty()) {
          status_ = WRITE;
          DoSendResponse();
        } else if (server_streaming_done_) {
          DoFinish();
        }
      }
    }
  }

  void DoFinish() {
    status_ = FINISH;
    AsyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    responder_.Finish(grpc::Status::OK, this);
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
      --server_streaming_rpc_counter;
      LOG(INFO) << "pending server streaming rpcs count = "
                << server_streaming_rpc_counter;
      handler_.Done(this, server_context_.IsCancelled());
      delete this;
    }
  }

public:
  static std::atomic<int32_t> server_streaming_rpc_counter;

private:
  ServiceType *async_service_;
  typename ThisRpcTypeHandler::GRPCResponder responder_;
  ThisRpcTypeHandler handler_;
  RequestType request_;
  ResponseType response_;
  std::list<ResponseType> response_list_;
  bool server_streaming_done_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_async_service_RPC_SERVER_STREAMING_RPC_H

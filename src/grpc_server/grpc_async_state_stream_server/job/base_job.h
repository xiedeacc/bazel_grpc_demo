/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef grpc_server_RPC_BASE_RPC_H
#define grpc_server_RPC_BASE_RPC_H
#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>
#pragma once

#include "glog/logging.h"
#include "grpc++/grpc++.h"
namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace job {

class BaseJob {
public:
  enum AsyncOpType {
    ASYNC_OP_TYPE_INVALID,
    ASYNC_OP_TYPE_QUEUED_REQUEST,
    ASYNC_OP_TYPE_READ,
    ASYNC_OP_TYPE_WRITE,
    ASYNC_OP_TYPE_FINISH
  };

  enum ProcessStatus { CREATE, INIT, READ, PROCESS, WRITE, FINISH };

  BaseJob(grpc::ServerCompletionQueue *request_queue,
          grpc::ServerCompletionQueue *response_queue)
      : async_op_counter_(0), async_read_in_progress_(false),
        async_write_in_progress_(false), fail_rate_(0),
        request_queue_(request_queue), response_queue_(response_queue),
        status_(CREATE) {}

  virtual ~BaseJob(){
      // report fail_rate_
  };

  const ProcessStatus GetStatus() { return status_; }

  bool FinishWithError(const grpc::Status &error) {
    return FinishWithErrorImpl(error);
  }

  virtual void SetFailed() { fail_rate_ = 10000; }

  virtual void Done() = 0;

  virtual void Proceed(bool ok) {
    switch (status_) {
    case CREATE:
      RequestRpc(ok);
      break;
    case INIT:
      Init(ok);
      break;
    case READ:
      ReadRequest(ok);
      break;
    case PROCESS:
      HandleRequest(ok);
      break;
    case WRITE:
      break;
    case FINISH:
      OnFinish(ok);
    default:;
      Done();
    }
  }

protected:
  virtual void RequestRpc(bool ok) = 0;

  virtual void Init(bool ok) = 0;

  virtual void ReadRequest(bool ok) = 0;

  virtual void HandleRequest(bool ok) = 0;

  virtual void OnFinish(bool ok) {
    LOG(INFO) << "OnFinish";
    AsyncOpFinished(BaseJob::ASYNC_OP_TYPE_FINISH);
  };

  virtual bool FinishWithErrorImpl(const grpc::Status &error) = 0;

  void AsyncOpStarted(AsyncOpType opType) {
    ++async_op_counter_;

    switch (opType) {
    case ASYNC_OP_TYPE_READ:
      async_read_in_progress_ = true;
      break;
    case ASYNC_OP_TYPE_WRITE:
      async_write_in_progress_ = true;
    default: // Don't care about other ops
      break;
    }
  }

  bool AsyncOpFinished(AsyncOpType opType) {
    --async_op_counter_;

    switch (opType) {
    case ASYNC_OP_TYPE_READ:
      async_read_in_progress_ = false;
      break;
    case ASYNC_OP_TYPE_WRITE:
      async_write_in_progress_ = false;
    default: // Don't care about other ops
      break;
    }

    return true;
  }

  bool AsyncOpInProgress() const { return async_op_counter_ != 0; }

  bool AsyncReadInProgress() const { return async_read_in_progress_; }

  bool AsyncWriteInProgress() const { return async_write_in_progress_; }

protected:
  int32_t async_op_counter_;
  bool async_read_in_progress_;
  bool async_write_in_progress_;
  double fail_rate_;

protected:
  grpc::ServerCompletionQueue *request_queue_;
  grpc::ServerCompletionQueue *response_queue_;
  ProcessStatus status_; // The current serving state.

public:
  grpc::ServerContext server_context_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // grpc_server_RPC_BASE_RPC_H

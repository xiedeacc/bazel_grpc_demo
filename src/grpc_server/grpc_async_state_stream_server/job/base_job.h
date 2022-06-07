/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef grpc_server_RPC_BASE_RPC_H
#define grpc_server_RPC_BASE_RPC_H
#pragma once
#include <grpcpp/completion_queue.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/server_context.h>

#include "glog/logging.h"
#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/job_done_handler.h"
#include "src/grpc_server/grpc_async_state_stream_server/job/job_interface.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace job {

class BaseJob : public JobInterface {
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
        status_(CREATE), job_done_handler_(this), on_done_called_(false) {}

  virtual ~BaseJob(){
      // report fail_rate_
  };

  const ProcessStatus GetStatus() { return status_; }

  bool FinishWithError(const grpc::Status &error) {
    return FinishWithErrorImpl(error);
  }

  virtual void SetFailed() { fail_rate_ = 10000; }

  virtual void Done() = 0;

  virtual void OnDone(bool ok) {
    on_done_called_ = true;
    LOG(INFO) << "OnDone called, async_op_counter_ = " << async_op_counter_;
    if (async_op_counter_ == 0) {
      Done();
    }
  }

  virtual void Proceed(bool ok) {
    switch (status_) {
    case CREATE:
      RequestRpc(ok);
      break;
    case INIT:
      // LOG(INFO) << "INIT";
      Init(ok);
      break;
    case READ:
      // LOG(INFO) << "READ";
      ReadRequest(ok);
      break;
    case PROCESS:
      // LOG(INFO) << "PROCESS";
      HandleRequest(ok);
      break;
    case WRITE:
      WriteNext(ok);
      break;
    case FINISH:
      // LOG(INFO) << "FINISH";
      OnFinish(ok);
    }
  }

  bool AsyncOpInProgress() const { return async_op_counter_ > 0; }

  virtual bool SendResponse(const google::protobuf::Message *response_msg) = 0;

protected:
  virtual void RequestRpc(bool ok) = 0;

  virtual void Init(bool ok) = 0;

  virtual void ReadRequest(bool ok) {
    LOG(ERROR) << "should never called!";
    exit(-1);
  }

  virtual void HandleRequest(bool ok) {
    LOG(ERROR) << "should never called!";
    exit(-1);
  }

  virtual void WriteNext(bool ok) {
    LOG(ERROR) << "should never called!";
    exit(-1);
  };

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
    if (async_op_counter_ == 0 && on_done_called_) {
      Done();
      return false;
    }
    return true;
  }

  bool AsyncReadInProgress() const { return async_read_in_progress_; }

  bool AsyncWriteInProgress() const { return async_write_in_progress_; }

protected:
  int32_t async_op_counter_;
  bool async_read_in_progress_;
  bool async_write_in_progress_;
  double fail_rate_;
  JobDoneHandler job_done_handler_;
  bool on_done_called_;

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

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef JOB_BASE_JOB_H
#define JOB_BASE_JOB_H
#pragma once

#include <grpcpp/completion_queue.h>
#include <grpcpp/server_context.h>

#include "glog/logging.h"
#include "grpc++/grpc++.h"

namespace grpc_demo {
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

  BaseJob()
      : mAsyncOpCounter(0), mAsyncReadInProgress(false),
        mAsyncWriteInProgress(false), mOnDoneCalled(false) {}

  virtual ~BaseJob(){};

  const grpc::ServerContext &getServerContext() const { return mServerContext; }
  bool sendResponse(const google::protobuf::Message *response) {
    return sendResponseImpl(response);
  }

  // This should be called for system level errors when no response is available
  bool finishWithError(const grpc::Status &error) {
    return finishWithErrorImpl(error);
  }

protected:
  virtual bool sendResponseImpl(const google::protobuf::Message *response) = 0;
  virtual bool finishWithErrorImpl(const grpc::Status &error) = 0;

  void asyncOpStarted(AsyncOpType opType) {
    ++mAsyncOpCounter;

    switch (opType) {
    case ASYNC_OP_TYPE_READ:
      mAsyncReadInProgress = true;
      break;
    case ASYNC_OP_TYPE_WRITE:
      mAsyncWriteInProgress = true;
    default: // Don't care about other ops
      break;
    }
  }

  // returns true if the rpc processing should keep going. false otherwise.
  bool asyncOpFinished(AsyncOpType opType) {
    --mAsyncOpCounter;

    switch (opType) {
    case ASYNC_OP_TYPE_READ:
      mAsyncReadInProgress = false;
      break;
    case ASYNC_OP_TYPE_WRITE:
      mAsyncWriteInProgress = false;
    default: // Don't care about other ops
      break;
    }

    // No async operations are pending and gRPC library notified as earlier that
    // it is done with the rpc. Finish the rpc.
    if (mAsyncOpCounter == 0 && mOnDoneCalled) {
      done();
      return false;
    }

    return true;
  }

  bool asyncOpInProgress() const { return mAsyncOpCounter != 0; }

  bool asyncReadInProgress() const { return mAsyncReadInProgress; }

  bool asyncWriteInProgress() const { return mAsyncWriteInProgress; }

public:
  // Tag processor for the 'done' event of this rpc from gRPC library
  void onDone(bool /*ok*/) {
    mOnDoneCalled = true;
    if (mAsyncOpCounter == 0)
      done();
  }

  // Each different rpc type need to implement the specialization of action when
  // this rpc is done.
  virtual void done() = 0;

private:
  int32_t mAsyncOpCounter;
  bool mAsyncReadInProgress;
  bool mAsyncWriteInProgress;

  // In case of an abrupt rpc ending (for example, client process exit), gRPC
  // calls OnDone prematurely even while an async operation is in progress and
  // would be notified later. An example sequence would be
  // 1. The client issues an rpc request.
  // 2. The server handles the rpc and calls Finish with response. At this
  // point, ServerContext::IsCancelled is NOT true.
  // 3. The client process abruptly exits.
  // 4. The completion queue dispatches an OnDone tag followed by the OnFinish
  // tag. If the application cleans up the state in OnDone, OnFinish invocation
  // would result in undefined behavior. This actually feels like a pretty odd
  // behavior of the gRPC library (it is most likely a result of our
  // multi-threaded usage) so we account for that by keeping track of whether
  // the OnDone was called earlier. As far as the application is considered, the
  // rpc is only 'done' when no asyn Ops are pending.
  bool mOnDoneCalled;

protected:
  // The application can use the ServerContext for taking into account the
  // current 'situation' of the rpc.
  grpc::ServerContext mServerContext;
};

} // namespace job
} // namespace grpc_demo

#endif // JOB_BASE_JOB_H

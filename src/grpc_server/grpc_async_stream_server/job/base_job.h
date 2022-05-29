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
#include "google/protobuf/service.h"
#include "grpc++/grpc++.h"

namespace grpc_demo {
namespace grpc_async_stream_server {
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
      break;
    }
  }

  bool asyncOpFinished(AsyncOpType opType) {
    --mAsyncOpCounter;

    switch (opType) {
    case ASYNC_OP_TYPE_READ:
      mAsyncReadInProgress = false;
      break;
    case ASYNC_OP_TYPE_WRITE:
      mAsyncWriteInProgress = false;
      break;
    }

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
  void onDone(bool /*ok*/) {
    mOnDoneCalled = true;
    LOG(INFO) << "onDone called, mAsyncOpCounter = " << mAsyncOpCounter;
    if (mAsyncOpCounter == 0)
      done();
  }

  virtual void done() = 0;

private:
  int32_t mAsyncOpCounter;
  bool mAsyncReadInProgress;
  bool mAsyncWriteInProgress;

  bool mOnDoneCalled;

protected:
  grpc::ServerContext mServerContext;
};

} // namespace job
} // namespace grpc_async_stream_server
} // namespace grpc_demo

#endif

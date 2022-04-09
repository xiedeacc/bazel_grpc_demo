/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef JOB_SERVER_STREAMING_JOB_H
#define JOB_SERVER_STREAMING_JOB_H
#pragma once

#include <grpcpp/completion_queue.h>

#include "src/grpc_server/handler/base_handler.h"
#include "src/grpc_server/handler/bidirectional_streaming_handler.h"
#include "src/grpc_server/handler/client_streaming_handler.h"
#include "src/grpc_server/handler/server_streaming_handler.h"
#include "src/grpc_server/handler/unary_handler.h"
#include "src/grpc_server/job/base_job.h"

namespace grpc_demo {
namespace grpc_server {
namespace job {

template <typename ServiceType, typename RequestType, typename ResponseType>
class ServerStreamingJob : public BaseJob {
  using ThisJobTypeHandlers =
      grpc_demo::grpc_server::handler::ServerStreamingHandlers<
          ServiceType, RequestType, ResponseType>;

public:
  ServerStreamingJob(ServiceType *service, grpc::ServerCompletionQueue *cq,
                     ThisJobTypeHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mServerStreamingDone(false) {
    ++gServerStreamingJobCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnRead =
        std::bind(&ServerStreamingJob::onRead, this, std::placeholders::_1);
    mOnWrite =
        std::bind(&ServerStreamingJob::onWrite, this, std::placeholders::_1);
    mOnFinish =
        std::bind(&ServerStreamingJob::onFinish, this, std::placeholders::_1);
    mOnDone = std::bind(&BaseJob::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
    mHandlers.requestRpc(mService, &mServerContext, &mRequest, &mResponder, mCQ,
                         mCQ, &mOnRead);
  }

private:
  // gRPC can only do one async write at a time but that is very inconvenient
  // from the application point of view. So we buffer the response below in a
  // queue if gRPC lib is not ready for it. The application can send a null
  // response in order to indicate the completion of server side streaming.
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);

    if (response != nullptr) {
      mResponseQueue.push_back(*response);

      if (!asyncWriteInProgress()) {
        doSendResponse();
      }
    } else {
      mServerStreamingDone = true;

      if (!asyncWriteInProgress()) {
        doFinish();
      }
    }

    return true;
  }

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(error, &mOnFinish);

    return true;
  }

  void doSendResponse() {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_WRITE);
    mResponder.Write(mResponseQueue.front(), &mOnWrite);
  }

  void doFinish() {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(grpc::Status::OK, &mOnFinish);
  }

  void onRead(bool ok) {
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        mHandlers.processIncomingRequest(*this, &mRequest);
      }
    }
  }

  void onWrite(bool ok) {
    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_WRITE)) {
      // Get rid of the message that just finished.
      mResponseQueue.pop_front();

      if (ok) {
        if (!mResponseQueue.empty()) // If we have more messages waiting to be
                                     // sent, send them.
        {
          doSendResponse();
        } else if (mServerStreamingDone) // Previous write completed and we did
                                         // not have any pending write. If the
                                         // application has finished streaming
                                         // responses, finish the rpc
                                         // processing.
        {
          doFinish();
        }
      }
    }
  }

  void onFinish(bool ok) { asyncOpFinished(BaseJob::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gServerStreamingJobCounter;
    LOG(INFO) << "Pending Server Streaming Rpcs Count = "
              << gServerStreamingJobCounter;
  }

public:
  static std::atomic<int32_t> gServerStreamingJobCounter;

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisJobTypeHandlers::GRPCResponder mResponder;

  RequestType mRequest;

  ThisJobTypeHandlers mHandlers;

  std::function<void(bool)> mOnRead;
  std::function<void(bool)> mOnWrite;
  std::function<void(bool)> mOnFinish;
  std::function<void(bool)> mOnDone;

  std::list<ResponseType> mResponseQueue;
  bool mServerStreamingDone;
};

} // namespace job
} // namespace grpc_server
} // namespace grpc_demo

#endif // JOB_SERVER_STREAMING_JOB_H

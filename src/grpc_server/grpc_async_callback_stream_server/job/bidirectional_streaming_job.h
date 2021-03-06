/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef JOB_BI_STREAMING_JOB_H
#define JOB_BI_STREAMING_JOB_H
#pragma once

#include <cstdlib>

#include <grpcpp/completion_queue.h>

#include "src/grpc_server/grpc_async_callback_stream_server/handler/base_handler.h"
#include "src/grpc_server/grpc_async_callback_stream_server/handler/bidirectional_streaming_handler.h"
#include "src/grpc_server/grpc_async_callback_stream_server/handler/client_streaming_handler.h"
#include "src/grpc_server/grpc_async_callback_stream_server/handler/server_streaming_handler.h"
#include "src/grpc_server/grpc_async_callback_stream_server/handler/unary_handler.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/base_job.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {
namespace job {

template <typename ServiceType, typename RequestType, typename ResponseType>
class BidirectionalStreamingJob : public BaseJob {
  using ThisJobTypeHandlers =
      grpc_demo::grpc_server::grpc_async_callback_stream_server::handler::
          BidirectionalStreamingHandlers<ServiceType, RequestType,
                                         ResponseType>;

public:
  BidirectionalStreamingJob(ServiceType *service,
                            grpc::ServerCompletionQueue *cq,
                            ThisJobTypeHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mServerStreamingDone(false),
        mClientStreamingDone(false) {
    ++gBidirectionalStreamingJobCounter;
    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnInit = std::bind(&BidirectionalStreamingJob::onInit, this,
                        std::placeholders::_1);
    mOnRead = std::bind(&BidirectionalStreamingJob::onRead, this,
                        std::placeholders::_1);
    mOnWrite = std::bind(&BidirectionalStreamingJob::onWrite, this,
                         std::placeholders::_1);
    mOnFinish = std::bind(&BidirectionalStreamingJob::onFinish, this,
                          std::placeholders::_1);
    mOnDone = std::bind(&BaseJob::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
    LOG(INFO) << "created!";
    LOG(INFO) << "Pending Bidirectional Streaming Rpcs Count = "
              << gBidirectionalStreamingJobCounter;
    mHandlers.requestRpc(mService, &mServerContext, &mResponder, mCQ, mCQ,
                         &mOnInit);
    LOG(INFO) << "requestRpc";
  }

private:
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);
    // LOG(INFO) << "response: "
    //<< (response == nullptr ? "nullptr" : "not nullptr");
    if (response == nullptr && !mClientStreamingDone) {
      GPR_ASSERT(false); // If you want to cancel, use BaseJob::finishWithError
      return false;
    }

    if (response != nullptr) {
      mResponseQueue.push_back(
          *response); // We need to make a copy of the response because we need
                      // to maintain it until we get a completion notification.

      if (!asyncWriteInProgress()) {
        doSendResponse();
      }
    } else {
      mServerStreamingDone = true;

      if (!asyncWriteInProgress()) // Kick the async op if our state machine is
                                   // not going to be kicked from the completion
                                   // queue
      {
        doFinish();
      }
    }

    return true;
  }

  void doSendResponse() {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_WRITE);
    mResponder.Write(mResponseQueue.front(), &mOnWrite);
  }

  void doFinish() {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    LOG(INFO) << "doFinish";
    mResponder.Finish(grpc::Status::OK, &mOnFinish);
  }

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(error, &mOnFinish);

    return true;
  }

  void onInit(bool ok) {
    mHandlers.createRpc();
    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        asyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        LOG(INFO) << "read";
        mResponder.Read(&mRequest, &mOnRead);
      }
    }
  }

  void onRead(bool ok) {
    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        // inform application that a new request has come in

        // std::this_thread::sleep_for(std::chrono::milliseconds{dist(generator)});
        LOG(INFO) << "processIncomingRequest";
        mHandlers.processIncomingRequest(*this, &mRequest);

        // queue up another read operation for this rpc
        asyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        LOG(INFO) << "read";
        mResponder.Read(&mRequest, &mOnRead);
      } else {
        mClientStreamingDone = true;
        LOG(INFO) << "processIncomingRequest"
                  << (mClientStreamingDone ? " true" : " false");
        mHandlers.processIncomingRequest(*this, nullptr);
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
                                         // application indicated a done
                                         // operation, finish the rpc
                                         // processing.
        {
          doFinish();
        }
      }
    }
  }

  void onFinish(bool ok) {
    LOG(INFO) << "onFinish";
    asyncOpFinished(BaseJob::ASYNC_OP_TYPE_FINISH);
  }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gBidirectionalStreamingJobCounter;
    LOG(INFO) << "Pending Bidirectional Streaming Rpcs Count = "
              << gBidirectionalStreamingJobCounter;
  }

public:
  static std::atomic<int32_t> gBidirectionalStreamingJobCounter;

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisJobTypeHandlers::GRPCResponder mResponder;

  RequestType mRequest;

  ThisJobTypeHandlers mHandlers;

  std::function<void(bool)> mOnInit;
  std::function<void(bool)> mOnRead;
  std::function<void(bool)> mOnWrite;
  std::function<void(bool)> mOnFinish;
  std::function<void(bool)> mOnDone;

  std::list<ResponseType> mResponseQueue;
  bool mServerStreamingDone;
  bool mClientStreamingDone;
};

} // namespace job
} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // JOB_BI_STREAMING_JOB_H

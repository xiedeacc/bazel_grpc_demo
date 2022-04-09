/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef JOB_CLIENT_STREAMING_JOB_H
#define JOB_CLIENT_STREAMING_JOB_H
#pragma once

#include <grpcpp/completion_queue.h>

#include "src/grpc_service/handler/base_handler.h"
#include "src/grpc_service/handler/bidirectional_streaming_handler.h"
#include "src/grpc_service/handler/client_streaming_handler.h"
#include "src/grpc_service/handler/server_streaming_handler.h"
#include "src/grpc_service/handler/unary_handler.h"
#include "src/grpc_service/job/base_job.h"

namespace grpc_demo {
namespace job {

template <typename ServiceType, typename RequestType, typename ResponseType>
class ClientStreamingJob : public BaseJob {
  using ThisJobTypeHandlers =
      ClientStreamingHandlers<ServiceType, RequestType, ResponseType>;

public:
  ClientStreamingJob(ServiceType *service, grpc::ServerCompletionQueue *cq,
                     ThisJobTypeHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mClientStreamingDone(false) {
    ++gClientStreamingJobCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnInit =
        std::bind(&ClientStreamingJob::onInit, this, std::placeholders::_1);
    mOnRead =
        std::bind(&ClientStreamingJob::onRead, this, std::placeholders::_1);
    mOnFinish =
        std::bind(&ClientStreamingJob::onFinish, this, std::placeholders::_1);
    mOnDone = std::bind(&BaseJob::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST);
    mHandlers.requestRpc(mService, &mServerContext, &mResponder, mCQ, mCQ,
                         &mOnInit);
  }

private:
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);

    GPR_ASSERT(
        response); // If no response is available, use BaseJob::finishWithError.

    if (response == nullptr)
      return false;

    if (!mClientStreamingDone) {
      // It does not make sense for server to finish the rpc before client has
      // streamed all the requests. Supporting this behavior could lead to
      // writing error-prone code so it is specifically disallowed.
      GPR_ASSERT(false); // If you want to cancel, use BaseJob::finishWithError
                         // with grpc::Cancelled status.
      return false;
    }

    mResponse = *response;

    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

    return true;
  }

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(BaseJob::ASYNC_OP_TYPE_FINISH);
    mResponder.FinishWithError(error, &mOnFinish);

    return true;
  }

  void onInit(bool ok) {
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        asyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      }
    }
  }

  void onRead(bool ok) {
    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        // inform application that a new request has come in
        mHandlers.processIncomingRequest(*this, &mRequest);

        // queue up another read operation for this rpc
        asyncOpStarted(BaseJob::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      } else {
        mClientStreamingDone = true;
        mHandlers.processIncomingRequest(*this, nullptr);
      }
    }
  }

  void onFinish(bool ok) { asyncOpFinished(BaseJob::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gClientStreamingJobCounter;
    gpr_log(GPR_DEBUG, "Pending Client Streaming Rpcs Count = %d",
            gClientStreamingJobCounter);
  }

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisJobTypeHandlers::GRPCResponder mResponder;

  RequestType mRequest;
  ResponseType mResponse;

  ThisJobTypeHandlers mHandlers;

  TagProcessor mOnInit;
  TagProcessor mOnRead;
  TagProcessor mOnFinish;
  TagProcessor mOnDone;

  bool mClientStreamingDone;
};

} // namespace job
} // namespace grpc_demo

#endif // JOB_CLIENT_STREAMING_JOB_H

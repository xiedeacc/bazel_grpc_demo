/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef JOB_UNARY_JOB_H
#define JOB_UNARY_JOB_H
#pragma once

#include <grpcpp/completion_queue.h>

#include "src/handler/base_handler.h"
#include "src/handler/bidirectional_streaming_handler.h"
#include "src/handler/client_streaming_handler.h"
#include "src/handler/server_streaming_handler.h"
#include "src/handler/unary_handler.h"
#include "src/job/base_job.h"

namespace grpc_demo {
namespace job {

template <typename ServiceType, typename RequestType, typename ResponseType>
class UnaryJob : public BaseJob {
  using ThisJobTypeHandlers =
      grpc_demo::handler::UnaryHandlers<ServiceType, RequestType, ResponseType>;

public:
  UnaryJob(ServiceType *service, grpc::ServerCompletionQueue *cq,
           ThisJobTypeHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers) {
    ++gUnaryJobCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnRead = std::bind(&UnaryJob::onRead, this, std::placeholders::_1);
    mOnFinish = std::bind(&UnaryJob::onFinish, this, std::placeholders::_1);
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
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);
    // If no response is available, use BaseJob::finishWithError.
    GPR_ASSERT(response);

    if (response == nullptr)
      return false;

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

  void onRead(bool ok) {
    // A request has come on the service which can now be handled. Create a new
    // rpc of this type to allow the server to handle next request.
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(BaseJob::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        // We have a request that can be responded to now. So process it.
        mHandlers.processIncomingRequest(*this, &mRequest);
      } else {
        GPR_ASSERT(ok);
      }
    }
  }

  void onFinish(bool ok) { asyncOpFinished(BaseJob::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gUnaryJobCounter;
    LOG(INFO) << "Pending Unary Rpcs Count = " << gUnaryJobCounter;
  }

public:
  static std::atomic<int32_t> gUnaryJobCounter;

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisJobTypeHandlers::GRPCResponder mResponder;

  RequestType mRequest;
  ResponseType mResponse;

  ThisJobTypeHandlers mHandlers;

  std::function<void(bool)> mOnRead;
  std::function<void(bool)> mOnFinish;
  std::function<void(bool)> mOnDone;
};

} // namespace job
} // namespace grpc_demo

#endif // JOB_UNARY_JOB_H

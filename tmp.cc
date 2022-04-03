/*
This example demonstrates RouteGuide Server example code implemented in Async
API fashion. The example below uses two threads. One thread is dedicated to the
completion queue processing of gRPC and another thread handles the completion
queue tags/events as they become available. The gRPC completion queue is put on
a separate thread in order to avoid blocking the main thread of the application
when no events are available in the completion queue.

The code below implements fully generic classes for the 4 different types of
rpcs found in gRPC (unary, server streaming, client streaming and bidirectional
streaming). They can be used by application to avoid writing all the state
management code with the gRPC. The code has been tested against version 1.0.0 of
the library.

The 4 different implementation classes duplicate code from each other - purely
as a matter of readability in this example. I implemnted a version without
duplication but that made the code considerably difficult to read
(comparatively) .
*/

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>

#include <grpc++/security/server_credentials.h>
#include <grpc++/server.h>
#include <grpc++/server_builder.h>
#include <grpc++/server_context.h>
#include <grpc++/support/async_stream.h>
#include <grpc/grpc.h>

#include "helper.h"
#include "route_guide.grpc.pb.h"

class ServerImpl;
// Globals to help with this example
static ServerImpl *gServerImpl;
std::string gDB;

// Globals to analyze the results and make sure we are not leaking any rpcs
static std::atomic_int32_t gUnaryRpcCounter = 0;
static std::atomic_int32_t gServerStreamingRpcCounter = 0;
static std::atomic_int32_t gClientStreamingRpcCounter = 0;
static std::atomic_int32_t gBidirectionalStreamingRpcCounter = 0;

// We add a 'TagProcessor' to the completion queue for each event. This way,
// each tag knows how to process itself.
using TagProcessor = std::function<void(bool)>;
struct TagInfo {
  TagProcessor
      *tagProcessor; // The function to be called to process incoming event
  bool ok; // The result of tag processing as indicated by gRPC library. Calling
           // it 'ok' to be in sync with other gRPC examples.
};

using TagList = std::list<TagInfo>;

// As the tags become available from completion queue thread, we put them in a
// queue in order to process them on our application thread.
static TagList gIncomingTags;
std::mutex gIncomingTagsMutex;

// Random sleep code in order to introduce some randomness in this example. This
// allows for quick stress testing.
static void randomSleepThisThread(int lowerBoundMS, int upperBoundMS) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<> dist{lowerBoundMS, upperBoundMS};
  std::this_thread::sleep_for(std::chrono::milliseconds{dist(generator)});
}

static void randomSleepThisThread() { randomSleepThisThread(10, 100); }

float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

float GetDistance(const routeguide::Point &start,
                  const routeguide::Point &end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

  float a = pow(sin(delta_lat_rad / 2), 2) +
            cos(lat_rad_1) * cos(lat_rad_2) * pow(sin(delta_lon_rad / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  int R = 6371000; // metres

  return R * c;
}

std::string
GetFeatureName(const routeguide::Point &point,
               const std::vector<routeguide::Feature> &feature_list) {
  for (const routeguide::Feature &f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

// A base class for various rpc types. With gRPC, it is necessary to keep track
// of pending async operations. Only 1 async operation can be pending at a time
// with an exception that both async read and write can be pending at the same
// time.

class RpcBase {
public:
  enum AsyncOpType {
    ASYNC_OP_TYPE_INVALID,
    ASYNC_OP_TYPE_QUEUED_REQUEST,
    ASYNC_OP_TYPE_READ,
    ASYNC_OP_TYPE_WRITE,
    ASYNC_OP_TYPE_FINISH
  };

  RpcBase()
      : mAsyncOpCounter(0), mAsyncReadInProgress(false),
        mAsyncWriteInProgress(false), mOnDoneCalled(false) {}

  virtual ~RpcBase(){};

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

// The application code communicates with our utility classes using these
// handlers.

// RpcJobHandlers is a base class that has signature of functions that are
// common for all the rpc types. The "rpc request call" signature is different
// for each rpc type so that lives in the derived class.

// typedefs. See the comments below.
using CreateRpc =
    std::function<void(grpc::Service *, grpc::ServerCompletionQueue *)>;

using ProcessIncomingRequest =
    std::function<void(RpcBase &, const google::protobuf::Message *)>;
using Done = std::function<void(RpcBase &, bool)>;

template <typename ServiceType, typename RequestType, typename ResponseType>
struct RpcHandlers {
public:
  // In gRPC async model, an application has to explicitly ask the gRPC server
  // to start handling an incoming rpc on a particular service. createRpc is
  // called when an outstanding RpcBase starts serving an incoming rpc and we
  // need to create the next rpc of this type to service further incoming rpcs.
  CreateRpc createRpc;

  // A new request has come in for this rpc. processIncomingRequest is called to
  // handle it. Note that with streaming rpcs, a request can come in multiple
  // times.
  ProcessIncomingRequest processIncomingRequest;

  // The gRPC server is done with this Rpc. Any necessary clean up can be done
  // when done is called.
  Done done;
};

// Each rpc type specializes RpcJobHandlers by deriving from it as each of them
// have a different responder to talk back to gRPC library.
template <typename ServiceType, typename RequestType, typename ResponseType>
struct UnaryRpcHandlers
    : public RpcHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder = grpc::ServerAsyncResponseWriter<ResponseType>;
  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, RequestType *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  // The actual queuing function on the generated service. This is called when
  // an instance of rpc job is created.
  RequestRpc requestRpc;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
struct ServerStreamingRpcHandlers
    : public RpcHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder = grpc::ServerAsyncWriter<ResponseType>;
  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, RequestType *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  // The actual queuing function on the generated service. This is called when
  // an instance of rpc job is created.
  RequestRpc requestRpc;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
struct ClientStreamingRpcHandlers
    : public RpcHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder = grpc::ServerAsyncReader<ResponseType, RequestType>;
  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  // The actual queuing function on the generated service. This is called when
  // an instance of rpc job is created.
  RequestRpc requestRpc;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
struct BidirectionalStreamingRpcHandlers
    : public RpcHandlers<ServiceType, RequestType, ResponseType> {
public:
  using GRPCResponder =
      grpc::ServerAsyncReaderWriter<ResponseType, RequestType>;
  using RequestRpc = std::function<void(
      ServiceType *, grpc::ServerContext *, GRPCResponder *,
      grpc::CompletionQueue *, grpc::ServerCompletionQueue *, void *)>;

  // The actual queuing function on the generated service. This is called when
  // an instance of rpc job is created.
  RequestRpc requestRpc;
};

/*
We implement UnaryRpcJob, ServerStreamingRpcJob, ClientStreamingRpcJob and
BidirectionalStreamingRpcJob. The application deals with these classes.

As a convention, we always send grpc::Status::OK and add any error info in the
google.rpc.Status member of the ResponseType field. In streaming scenarios, this
allows us to indicate error in a request to a client without completion of the
rpc (and allow for more requests on same rpc). We do, however, allow server side
cancellation of the rpc.
*/

template <typename ServiceType, typename RequestType, typename ResponseType>
class UnaryRpc : public RpcBase {
  using ThisRpcTypeJobHandlers =
      UnaryRpcHandlers<ServiceType, RequestType, ResponseType>;

public:
  UnaryRpc(ServiceType *service, grpc::ServerCompletionQueue *cq,
           ThisRpcTypeJobHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers) {
    ++gUnaryRpcCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnRead = std::bind(&UnaryRpc::onRead, this, std::placeholders::_1);
    mOnFinish = std::bind(&UnaryRpc::onFinish, this, std::placeholders::_1);
    mOnDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST);
    mHandlers.requestRpc(mService, &mServerContext, &mRequest, &mResponder, mCQ,
                         mCQ, &mOnRead);
  }

private:
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);

    GPR_ASSERT(
        response); // If no response is available, use RpcBase::finishWithError.

    if (response == nullptr)
      return false;

    mResponse = *response;

    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

    return true;
  }

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.FinishWithError(error, &mOnFinish);

    return true;
  }

  void onRead(bool ok) {
    // A request has come on the service which can now be handled. Create a new
    // rpc of this type to allow the server to handle next request.
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        // We have a request that can be responded to now. So process it.
        mHandlers.processIncomingRequest(*this, &mRequest);
      } else {
        GPR_ASSERT(ok);
      }
    }
  }

  void onFinish(bool ok) { asyncOpFinished(RpcBase::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gUnaryRpcCounter;
    gpr_log(GPR_DEBUG, "Pending Unary Rpcs Count = %d", gUnaryRpcCounter);
  }

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;

  RequestType mRequest;
  ResponseType mResponse;

  ThisRpcTypeJobHandlers mHandlers;

  TagProcessor mOnRead;
  TagProcessor mOnFinish;
  TagProcessor mOnDone;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
class ServerStreamingRpc : public RpcBase {
  using ThisRpcTypeJobHandlers =
      ServerStreamingRpcHandlers<ServiceType, RequestType, ResponseType>;

public:
  ServerStreamingRpc(ServiceType *service, grpc::ServerCompletionQueue *cq,
                     ThisRpcTypeJobHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mServerStreamingDone(false) {
    ++gServerStreamingRpcCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnRead =
        std::bind(&ServerStreamingRpc::onRead, this, std::placeholders::_1);
    mOnWrite =
        std::bind(&ServerStreamingRpc::onWrite, this, std::placeholders::_1);
    mOnFinish =
        std::bind(&ServerStreamingRpc::onFinish, this, std::placeholders::_1);
    mOnDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST);
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
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(error, &mOnFinish);

    return true;
  }

  void doSendResponse() {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_WRITE);
    mResponder.Write(mResponseQueue.front(), &mOnWrite);
  }

  void doFinish() {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(grpc::Status::OK, &mOnFinish);
  }

  void onRead(bool ok) {
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        mHandlers.processIncomingRequest(*this, &mRequest);
      }
    }
  }

  void onWrite(bool ok) {
    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_WRITE)) {
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

  void onFinish(bool ok) { asyncOpFinished(RpcBase::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gServerStreamingRpcCounter;
    gpr_log(GPR_DEBUG, "Pending Server Streaming Rpcs Count = %d",
            gServerStreamingRpcCounter);
  }

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;

  RequestType mRequest;

  ThisRpcTypeJobHandlers mHandlers;

  TagProcessor mOnRead;
  TagProcessor mOnWrite;
  TagProcessor mOnFinish;
  TagProcessor mOnDone;

  std::list<ResponseType> mResponseQueue;
  bool mServerStreamingDone;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
class ClientStreamingRpc : public RpcBase {
  using ThisRpcTypeJobHandlers =
      ClientStreamingRpcHandlers<ServiceType, RequestType, ResponseType>;

public:
  ClientStreamingRpc(ServiceType *service, grpc::ServerCompletionQueue *cq,
                     ThisRpcTypeJobHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mClientStreamingDone(false) {
    ++gClientStreamingRpcCounter;

    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnInit =
        std::bind(&ClientStreamingRpc::onInit, this, std::placeholders::_1);
    mOnRead =
        std::bind(&ClientStreamingRpc::onRead, this, std::placeholders::_1);
    mOnFinish =
        std::bind(&ClientStreamingRpc::onFinish, this, std::placeholders::_1);
    mOnDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST);
    mHandlers.requestRpc(mService, &mServerContext, &mResponder, mCQ, mCQ,
                         &mOnInit);
  }

private:
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);

    GPR_ASSERT(
        response); // If no response is available, use RpcBase::finishWithError.

    if (response == nullptr)
      return false;

    if (!mClientStreamingDone) {
      // It does not make sense for server to finish the rpc before client has
      // streamed all the requests. Supporting this behavior could lead to
      // writing error-prone code so it is specifically disallowed.
      GPR_ASSERT(false); // If you want to cancel, use RpcBase::finishWithError
                         // with grpc::Cancelled status.
      return false;
    }

    mResponse = *response;

    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(mResponse, grpc::Status::OK, &mOnFinish);

    return true;
  }

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.FinishWithError(error, &mOnFinish);

    return true;
  }

  void onInit(bool ok) {
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        asyncOpStarted(RpcBase::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      }
    }
  }

  void onRead(bool ok) {
    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        // inform application that a new request has come in
        mHandlers.processIncomingRequest(*this, &mRequest);

        // queue up another read operation for this rpc
        asyncOpStarted(RpcBase::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      } else {
        mClientStreamingDone = true;
        mHandlers.processIncomingRequest(*this, nullptr);
      }
    }
  }

  void onFinish(bool ok) { asyncOpFinished(RpcBase::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gClientStreamingRpcCounter;
    gpr_log(GPR_DEBUG, "Pending Client Streaming Rpcs Count = %d",
            gClientStreamingRpcCounter);
  }

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;

  RequestType mRequest;
  ResponseType mResponse;

  ThisRpcTypeJobHandlers mHandlers;

  TagProcessor mOnInit;
  TagProcessor mOnRead;
  TagProcessor mOnFinish;
  TagProcessor mOnDone;

  bool mClientStreamingDone;
};

template <typename ServiceType, typename RequestType, typename ResponseType>
class BidirectionalStreamingRpc : public RpcBase {
  using ThisRpcTypeJobHandlers =
      BidirectionalStreamingRpcHandlers<ServiceType, RequestType, ResponseType>;

public:
  BidirectionalStreamingRpc(ServiceType *service,
                            grpc::ServerCompletionQueue *cq,
                            ThisRpcTypeJobHandlers jobHandlers)
      : mService(service), mCQ(cq), mResponder(&mServerContext),
        mHandlers(jobHandlers), mServerStreamingDone(false),
        mClientStreamingDone(false) {
    ++gBidirectionalStreamingRpcCounter;
    // create TagProcessors that we'll use to interact with gRPC CompletionQueue
    mOnInit = std::bind(&BidirectionalStreamingRpc::onInit, this,
                        std::placeholders::_1);
    mOnRead = std::bind(&BidirectionalStreamingRpc::onRead, this,
                        std::placeholders::_1);
    mOnWrite = std::bind(&BidirectionalStreamingRpc::onWrite, this,
                         std::placeholders::_1);
    mOnFinish = std::bind(&BidirectionalStreamingRpc::onFinish, this,
                          std::placeholders::_1);
    mOnDone = std::bind(&RpcBase::onDone, this, std::placeholders::_1);

    // set up the completion queue to inform us when gRPC is done with this rpc.
    mServerContext.AsyncNotifyWhenDone(&mOnDone);

    // finally, issue the async request needed by gRPC to start handling this
    // rpc.
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST);
    mHandlers.requestRpc(mService, &mServerContext, &mResponder, mCQ, mCQ,
                         &mOnInit);
  }

private:
  bool sendResponseImpl(const google::protobuf::Message *responseMsg) override {
    auto response = static_cast<const ResponseType *>(responseMsg);

    if (response == nullptr && !mClientStreamingDone) {
      // It does not make sense for server to finish the rpc before client has
      // streamed all the requests. Supporting this behavior could lead to
      // writing error-prone code so it is specifically disallowed.
      GPR_ASSERT(false); // If you want to cancel, use RpcBase::finishWithError
                         // with grpc::Cancelled status.
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

  bool finishWithErrorImpl(const grpc::Status &error) override {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(error, &mOnFinish);

    return true;
  }

  void onInit(bool ok) {
    mHandlers.createRpc(mService, mCQ);

    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_QUEUED_REQUEST)) {
      if (ok) {
        asyncOpStarted(RpcBase::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      }
    }
  }

  void onRead(bool ok) {
    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_READ)) {
      if (ok) {
        // inform application that a new request has come in
        mHandlers.processIncomingRequest(*this, &mRequest);

        // queue up another read operation for this rpc
        asyncOpStarted(RpcBase::ASYNC_OP_TYPE_READ);
        mResponder.Read(&mRequest, &mOnRead);
      } else {
        mClientStreamingDone = true;
        mHandlers.processIncomingRequest(*this, nullptr);
      }
    }
  }

  void onWrite(bool ok) {
    if (asyncOpFinished(RpcBase::ASYNC_OP_TYPE_WRITE)) {
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

  void onFinish(bool ok) { asyncOpFinished(RpcBase::ASYNC_OP_TYPE_FINISH); }

  void done() override {
    mHandlers.done(*this, mServerContext.IsCancelled());

    --gBidirectionalStreamingRpcCounter;
    gpr_log(GPR_DEBUG, "Pending Bidirectional Streaming Rpcs Count = %d",
            gBidirectionalStreamingRpcCounter);
  }

  void doSendResponse() {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_WRITE);
    mResponder.Write(mResponseQueue.front(), &mOnWrite);
  }

  void doFinish() {
    asyncOpStarted(RpcBase::ASYNC_OP_TYPE_FINISH);
    mResponder.Finish(grpc::Status::OK, &mOnFinish);
  }

private:
  ServiceType *mService;
  grpc::ServerCompletionQueue *mCQ;
  typename ThisRpcTypeJobHandlers::GRPCResponder mResponder;

  RequestType mRequest;

  ThisRpcTypeJobHandlers mHandlers;

  TagProcessor mOnInit;
  TagProcessor mOnRead;
  TagProcessor mOnWrite;
  TagProcessor mOnFinish;
  TagProcessor mOnDone;

  std::list<ResponseType> mResponseQueue;
  bool mServerStreamingDone;
  bool mClientStreamingDone;
};

class ServerImpl final {
public:
  ServerImpl() { routeguide::ParseDb(gDB, &mFeatureList); }

  ~ServerImpl() {
    mServer->Shutdown();
    // Always shutdown the completion queue after the server.
    mCQ->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&mRouteGuideService);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    mCQ = builder.AddCompletionQueue();
    // Finally assemble the server.
    mServer = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
  }

private:
  // Handlers for various rpcs. An application could do custom code generation
  // for creating these (except the actual processing logic).
  void createGetFeatureRpc() {
    UnaryRpcHandlers<routeguide::RouteGuide::AsyncService, routeguide::Point,
                     routeguide::Feature>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createGetFeatureRpc, this);

    rpcHandlers.processIncomingRequest = &GetFeatureProcessor;
    rpcHandlers.done = &GetFeatureDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestGetFeature;

    new UnaryRpc<routeguide::RouteGuide::AsyncService, routeguide::Point,
                 routeguide::Feature>(&mRouteGuideService, mCQ.get(),
                                      rpcHandlers);
  }

  static void GetFeatureProcessor(RpcBase &rpc,
                                  const google::protobuf::Message *message) {
    auto point = static_cast<const routeguide::Point *>(message);

    routeguide::Feature feature;
    feature.set_name(GetFeatureName(*point, gServerImpl->mFeatureList));
    feature.mutable_location()->CopyFrom(*point);

    randomSleepThisThread();
    rpc.sendResponse(&feature);
  }

  static void GetFeatureDone(RpcBase &rpc, bool rpcCancelled) { delete (&rpc); }

  void createListFeaturesRpc() {
    ServerStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                               routeguide::Rectangle, routeguide::Feature>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createListFeaturesRpc, this);

    rpcHandlers.processIncomingRequest = &ListFeaturesProcessor;
    rpcHandlers.done = &ListFeaturesDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestListFeatures;

    new ServerStreamingRpc<routeguide::RouteGuide::AsyncService,
                           routeguide::Rectangle, routeguide::Feature>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  static void ListFeaturesProcessor(RpcBase &rpc,
                                    const google::protobuf::Message *message) {
    auto rectangle = static_cast<const routeguide::Rectangle *>(message);

    auto lo = rectangle->lo();
    auto hi = rectangle->hi();
    long left = (std::min)(lo.longitude(), hi.longitude());
    long right = (std::max)(lo.longitude(), hi.longitude());
    long top = (std::max)(lo.latitude(), hi.latitude());
    long bottom = (std::min)(lo.latitude(), hi.latitude());
    for (auto f : gServerImpl->mFeatureList) {
      if (f.location().longitude() >= left &&
          f.location().longitude() <= right &&
          f.location().latitude() >= bottom && f.location().latitude() <= top) {
        rpc.sendResponse(&f);
        randomSleepThisThread();
      }
    }
    rpc.sendResponse(nullptr);
  }

  static void ListFeaturesDone(RpcBase &rpc, bool rpcCancelled) {
    delete (&rpc);
  }

  void createRecordRouteRpc() {
    ClientStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                               routeguide::Point, routeguide::RouteSummary>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createRecordRouteRpc, this);

    rpcHandlers.processIncomingRequest = &RecordRouteProcessor;
    rpcHandlers.done = &RecordRouteDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestRecordRoute;

    new ClientStreamingRpc<routeguide::RouteGuide::AsyncService,
                           routeguide::Point, routeguide::RouteSummary>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  struct RecordRouteState {
    int pointCount;
    int featureCount;
    float distance;
    routeguide::Point previous;
    std::chrono::system_clock::time_point startTime;
    RecordRouteState() : pointCount(0), featureCount(0), distance(0.0f) {}
  };

  std::unordered_map<RpcBase *, RecordRouteState> mRecordRouteMap;
  static void RecordRouteProcessor(RpcBase &rpc,
                                   const google::protobuf::Message *message) {
    auto point = static_cast<const routeguide::Point *>(message);

    RecordRouteState &state = gServerImpl->mRecordRouteMap[&rpc];

    if (point) {
      if (state.pointCount == 0)
        state.startTime = std::chrono::system_clock::now();

      state.pointCount++;
      if (!GetFeatureName(*point, gServerImpl->mFeatureList).empty()) {
        state.featureCount++;
      }
      if (state.pointCount != 1) {
        state.distance += GetDistance(state.previous, *point);
      }
      state.previous = *point;

      randomSleepThisThread();
    } else {
      std::chrono::system_clock::time_point endTime =
          std::chrono::system_clock::now();

      routeguide::RouteSummary summary;
      summary.set_point_count(state.pointCount);
      summary.set_feature_count(state.featureCount);
      summary.set_distance(static_cast<long>(state.distance));
      auto secs = std::chrono::duration_cast<std::chrono::seconds>(
          endTime - state.startTime);
      summary.set_elapsed_time(secs.count());
      rpc.sendResponse(&summary);

      gServerImpl->mRecordRouteMap.erase(&rpc);
      randomSleepThisThread();
    }
  }

  static void RecordRouteDone(RpcBase &rpc, bool rpcCancelled) {
    delete (&rpc);
  }

  void createRouteChatRpc() {
    BidirectionalStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                                      routeguide::RouteNote,
                                      routeguide::RouteNote>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createRouteChatRpc, this);

    rpcHandlers.processIncomingRequest = &RouteChatProcessor;
    rpcHandlers.done = &RouteChatDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestRouteChat;

    new BidirectionalStreamingRpc<routeguide::RouteGuide::AsyncService,
                                  routeguide::RouteNote, routeguide::RouteNote>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  static void RouteChatProcessor(RpcBase &rpc,
                                 const google::protobuf::Message *message) {
    auto note = static_cast<const routeguide::RouteNote *>(message);
    // Simply echo the note back.
    if (note) {
      routeguide::RouteNote responseNote(*note);
      rpc.sendResponse(&responseNote);
      randomSleepThisThread();
    } else {
      rpc.sendResponse(nullptr);
      randomSleepThisThread();
    }
  }

  static void RouteChatDone(RpcBase &rpc, bool rpcCancelled) { delete (&rpc); }

  void HandleRpcs() {
    createGetFeatureRpc();
    createListFeaturesRpc();
    createRecordRouteRpc();
    createRouteChatRpc();

    TagInfo tagInfo;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(mCQ->Next((void **)&tagInfo.tagProcessor,
                           &tagInfo.ok)); // GRPC_TODO - Handle returned value

      gIncomingTagsMutex.lock();
      gIncomingTags.push_back(tagInfo);
      gIncomingTagsMutex.unlock();
    }
  }

  std::unique_ptr<grpc::ServerCompletionQueue> mCQ;
  routeguide::RouteGuide::AsyncService mRouteGuideService;
  std::vector<routeguide::Feature> mFeatureList;
  std::unique_ptr<grpc::Server> mServer;
};

static void processRpcs() {
  // Implement a busy-wait loop. Not the most efficient thing in the world but
  // but would do for this example
  while (true) {
    gIncomingTagsMutex.lock();
    TagList tags = std::move(gIncomingTags);
    gIncomingTagsMutex.unlock();

    while (!tags.empty()) {
      TagInfo tagInfo = tags.front();
      tags.pop_front();
      (*(tagInfo.tagProcessor))(tagInfo.ok);

      randomSleepThisThread(); // Simulate processing Time
    };
    randomSleepThisThread(); // yield cpu
  }
}

int main(int argc, char **argv) {
  // Expect only arg:
  // --db_path=C:\work\gos-stream\mlclean\experimental\grpc-example\source/route_guide_db.json
  gDB = routeguide::GetDbFileContent(argc, argv);

  std::thread processorThread(processRpcs);

  gpr_set_log_verbosity(GPR_LOG_SEVERITY_DEBUG);

  ServerImpl server;
  gServerImpl = &server;
  server.Run();

  return 0;
}

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <atomic>
#include <memory>
#include <string>

#include "absl/time/clock.h"
#include "boost/thread/thread_pool.hpp"
#include "cat/cat.h"
#include "gflags/gflags.h"
#include "glog/logging.h"

#include "cmeli/common/module.h"
#include "cmeli/service/meli_embedding_data.h"
#include "cmeli/service/meli_rank_data.h"
#include "cmeli/service/meli_service_impl.h"
#include "cmeli/service/rest/http_meli_service.h"
#include "cmeli/util/config_util.h"
#include "cmeli/util/zk_node_manager.h"
#include "cmeli/util/zk_node_register.h"
#include "gperftools/profiler.h"

using absl::Milliseconds;
using absl::Now;
using absl::Seconds;
using absl::Time;
using absl::ToChronoTime;
using cat::Cat;
using cat::message::TransactionPtr;
using cmeli::service::CMeliService;
using cmeli::service::MeliCallData;
using cmeli::service::MeliEmbeddingData;
using cmeli::service::MeliRankData;
using cmeli::service::MeliServiceImpl;
using cmeli::service::MeliServiceImplPtr;
using cmeli::service::rest::HttpMeliService;
using cmeli::service::rest::HttpMeliServicePtr;

using grpc::CompletionQueue;
using grpc::InsecureServerCredentials;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using std::atomic;
using std::string;
using std::to_string;
using std::unique_ptr;

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

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <atomic>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include "boost/thread/thread_pool.hpp"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "src/grpc_server/grpc_async_callback_stream_server/server_impl.h"
#include "src/grpc_server/grpc_async_callback_stream_server/tag_info.h"

using grpc::CompletionQueue;
using grpc::InsecureServerCredentials;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerCompletionQueue;
using std::atomic;
using std::string;
using std::to_string;
using std::unique_ptr;

using TagList = std::list<
    grpc_demo::grpc_server::grpc_async_callback_stream_server::TagInfo>;
static void randomSleepThisThread(int lowerBoundMS, int upperBoundMS) {
  unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
  std::default_random_engine generator(seed);
  std::uniform_int_distribution<> dist{lowerBoundMS, upperBoundMS};
  std::this_thread::sleep_for(std::chrono::milliseconds{dist(generator)});
}

static void randomSleepThisThread() { randomSleepThisThread(10, 100); }

static void processRpcs(std::mutex *incoming_tags_mutex,
                        TagList *incoming_tags) {
  while (true) {
    // LOG(INFO) << incoming_tags->size() << std::endl;
    incoming_tags_mutex->lock();
    TagList tags = std::move(*incoming_tags);
    incoming_tags_mutex->unlock();

    while (!tags.empty()) {
      grpc_demo::grpc_server::grpc_async_callback_stream_server::TagInfo
          tagInfo = tags.front();
      tags.pop_front();
      (*(tagInfo.tagProcessor))(tagInfo.ok);
      randomSleepThisThread(); // Simulate processing Time
    };
    randomSleepThisThread(); // yield cpu
  }
}

int main(int argc, char **argv) {
  std::mutex incoming_tags_mutex;
  TagList incoming_tags;

  std::thread processorThread(processRpcs, &incoming_tags_mutex,
                              &incoming_tags);

  std::string db_content =
      grpc_demo::common::util::GetDbFileContent(argc, argv);
  grpc_demo::grpc_server::grpc_async_callback_stream_server::ServerImpl server(
      db_content, incoming_tags_mutex, incoming_tags);
  server.Run();

  return 0;
}

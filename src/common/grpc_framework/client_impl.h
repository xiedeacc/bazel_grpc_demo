//
// Created by zjf on 2018/3/9.
//

#ifndef GRPC_FRAMEWORK_CLIENT_IMPL_H
#define GRPC_FRAMEWORK_CLIENT_IMPL_H

#include "src/common/grpc_framework/tag_base.h"

#include "absl/time/clock.h"
#include "glog/logging.h"
#include <atomic>
#include <chrono>
#include <grpcpp/grpcpp.h>
#include <set>
#include <string>
#include <thread>

namespace grpc_demo {
namespace common {
namespace grpc_framework {

class ChannelStateCallback {
public:
  virtual void OnChannelStateChanged(grpc_connectivity_state old_state,
                                     grpc_connectivity_state new_state) = 0;
};

class ChannelStateMonitor : public TagBase {
public:
  ChannelStateMonitor(std::shared_ptr<grpc::Channel> channel,
                      grpc::CompletionQueue *cq, int minutes,
                      ChannelStateCallback *cb)
      : channel_(channel), cq_(cq), minutes_(minutes) {
    state_ = channel_->GetState(false);
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::minutes(minutes_);
    channel_->NotifyOnStateChange(state_, deadline, cq_, this);
    callback_ = cb;
  };

  virtual void Process() {
    auto current_state = channel_->GetState(false);
    LOG(INFO) << "channel state changed from " << state_ << "to "
              << current_state;
    callback_->OnChannelStateChanged(state_, current_state);
    state_ = current_state;
  };

  virtual void OnError() {
    LOG(INFO) << "ChannelStateMonitor time out, re-monite it";
    std::chrono::system_clock::time_point deadline =
        std::chrono::system_clock::now() + std::chrono::minutes(minutes_);
    channel_->NotifyOnStateChange(state_, deadline, cq_, this);
  };

  virtual std::string Name() { return "ChannelStateMonitor"; }

private:
  std::shared_ptr<grpc::Channel> channel_;
  grpc::CompletionQueue *cq_;
  int minutes_;
  grpc_connectivity_state state_;

  ChannelStateCallback *callback_;
};

template <typename ServiceType> class ClientImpl : public ChannelStateCallback {
public:
  typedef ClientImpl<ServiceType> this_type;

  ClientImpl() {}

  virtual std::shared_ptr<grpc::ChannelCredentials> GetCredental() {
    return grpc::InsecureChannelCredentials();
  }

  void Run(std::string address) {
    server_addr = address;
    running.store(true, std::memory_order_relaxed);
    thread_ = std::thread(&this_type::Srv, this);
    while (running.load()) {
      LOG(INFO) << "sleep 1 seconds";
      absl::SleepFor(absl::Seconds(1));
    }
  }

  virtual void OnRun(){};

  void Exit() {
    running.store(false, std::memory_order_relaxed);
    if (thread_.joinable()) {
      thread_.join();
    }
  }

  virtual void OnExit(){};

  typename ServiceType::Stub *stub() { return stub_.get(); }

  grpc::CompletionQueue *cq() { return cq_.get(); }

protected:
  void Srv() {
    while (true) {
      if (!running.load(std::memory_order_relaxed)) {
        cq_->Shutdown();
        OnExit();
        return;
      }

      credential = GetCredental();
      grpc::ChannelArguments channel_args;
      channel_args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);
      channel =
          grpc::CreateCustomChannel(server_addr, credential, channel_args);
      stub_ = ServiceType::NewStub(channel);
      cq_ = std::unique_ptr<grpc::CompletionQueue>(new grpc::CompletionQueue());

      std::chrono::system_clock::time_point deadline =
          std::chrono::system_clock::now() + std::chrono::seconds(5);
      if (!channel->WaitForConnected(deadline)) {
        LOG(INFO) << "channel connected failed, continue";
        continue;
      }

      ChannelStateMonitor_ = std::unique_ptr<ChannelStateMonitor>(
          new ChannelStateMonitor(channel, cq_.get(), 60 * 24, this));
      LOG(INFO) << "channel_state_listener_ is " << ChannelStateMonitor_.get();

      OnRun();

      void *got_tag;
      bool ok = false;
      while (cq_->Next(&got_tag, &ok)) {
        TagBase *call = static_cast<TagBase *>(got_tag);
        LOG(INFO) << "tag is " << got_tag << ", ok = " << ok
                  << ", name = " << call->Name();
        if (ok) {
          call->Process();
        } else {
          LOG(INFO) << "channele stats: " << channel->GetState(false);
          call->OnError();
        }
      }
      LOG(INFO) << "completion queue is shutting down. restart it";
      std::this_thread::yield();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000 * 10));
    }
  }

  virtual void OnChannelStateChanged(grpc_connectivity_state old_state,
                                     grpc_connectivity_state new_state) {
    if (new_state != GRPC_CHANNEL_READY) {
      LOG(INFO) << "shutdown cq";
      cq_->Shutdown();
    }
  }

protected:
  std::string server_addr;
  std::shared_ptr<grpc::Channel> channel;
  std::shared_ptr<grpc::ChannelCredentials> credential;

  std::unique_ptr<grpc::CompletionQueue> cq_;
  std::unique_ptr<typename ServiceType::Stub> stub_;

  std::atomic<bool> running;
  std::thread thread_;

  std::unique_ptr<ChannelStateMonitor> ChannelStateMonitor_;
};

} // namespace grpc_framework
} // namespace common
} // namespace grpc_demo
#endif // GRPC_FRAMEWORK_CLIENT_IMPL_H

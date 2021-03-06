//
// Created by zjf on 2018/2/12.
//

#ifndef GRPC_FRAMEWORK_SERVER_RPC_READER_H
#define GRPC_FRAMEWORK_SERVER_RPC_READER_H

#include "src/common/grpc_framework/tag_base.h"
#include <glog/logging.h>
#include <google/protobuf/arena.h>
#include <grpcpp/grpcpp.h>

namespace grpc_demo {
namespace common {
namespace grpc_framework {

template <typename RequestType, typename ReaderType> class Reader;

class ReaderCallback {
  template <typename RequestType, typename ReaderType> friend class Reader;

public:
  virtual void OnReadError() = 0;

  virtual void OnRead(void *req) = 0;
};

template <typename RequestType, typename ReaderType>
class Reader : public TagBase {
public:
  Reader(ReaderCallback *cb, ReaderType &async_reader)
      : callback_(*cb), reader_impl_(async_reader), auto_read_(true) {
    arena_ =
        std::unique_ptr<google::protobuf::Arena>(new google::protobuf::Arena());
  };

  virtual std::string Name() override { return "Reader"; }

  void SetAuto(bool auto_read) { auto_read_ = auto_read; }

  void Read() {
    gpr_log(GPR_DEBUG, "start reading");
    arena_->Reset();
    req_ = google::protobuf::Arena::Create<RequestType>(arena_.get());
    reader_impl_.Read(req_, this);
  };

  virtual void Process() {
    gpr_log(GPR_DEBUG, "end reading %p", this);
    callback_.OnRead((void *)req_);
    if (auto_read_) {
      Read();
    }
  };

  virtual void OnError() {
    LOG(ERROR) << "OnError";
    callback_.OnReadError();
  }

private:
  ReaderCallback &callback_;
  ReaderType &reader_impl_;
  bool auto_read_;
  RequestType *req_;

  std::unique_ptr<google::protobuf::Arena> arena_;
};

} // namespace grpc_framework
} // namespace common
} // namespace grpc_demo

#endif // GRPC_FRAMEWORK_SERVER_RPC_READER_H

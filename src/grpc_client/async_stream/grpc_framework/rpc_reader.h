//
// Created by zjf on 2018/2/12.
//

#ifndef GRPC_FRAMEWORK_RPC_READER_H
#define GRPC_FRAMEWORK_RPC_READER_H

#include <google/protobuf/arena.h>
#include <grpc/support/log.h>

#include "src/grpc_client/async_stream/grpc_framework/tag_base.h"

namespace grpc_framework {

template <typename ResponseType, typename ReaderType> class reader;

class ReaderCallback {
  template <typename ResponseType, typename ReaderType> friend class reader;

public:
  virtual void OnReadError() = 0;

  virtual void OnRead(void *req) = 0;
};

template <typename ResponseType, typename ReaderType>
class Reader : public TagBase {
public:
  Reader(ReaderCallback *cb, ReaderType &async_reader)
      : callback_(*cb), reader_impl_(async_reader), auto_read_(true) {
    arena_ =
        std::unique_ptr<google::protobuf::Arena>(new google::protobuf::Arena());
  };

  void SetAuto(bool auto_read) { auto_read_ = auto_read; }

  void Read() {
    gpr_log(GPR_DEBUG, "start reading");
    arena_->Reset();
    req_ = google::protobuf::Arena::Create<ResponseType>(arena_.get());
    reader_impl_.Read(req_, this);
  };

  virtual void Process() {
    gpr_log(GPR_DEBUG, "end reading %p", this);
    callback_.OnRead((void *)req_);
    if (auto_read_) {
      Read();
    }
  };

  virtual void OnError() { callback_.OnReadError(); }

private:
  ReaderCallback &callback_;
  ReaderType &reader_impl_;
  bool auto_read_;
  ResponseType *req_;

  std::unique_ptr<google::protobuf::Arena> arena_;
};

} // namespace grpc_framework

#endif // GRPC_FRAMEWORK_RPC_READER_H

//
// Created by zjf on 2018/2/12.
//

#ifndef GRPC_FRAMEWORK_RPC_WRITER_H
#define GRPC_FRAMEWORK_RPC_WRITER_H

#include "src/grpc_client/async_stream/grpc_framework/tag_base.h"
#include <google/protobuf/arena.h>
#include <grpc++/server.h>
#include <grpc/support/log.h>

#include <deque>
#include <list>
#include <thread>

namespace grpc_framework {

class WriterCallback {
public:
  virtual void OnWrite(int write_id) = 0;
  virtual void OnWriteError() = 0;
};

template <typename RequestType, typename WriterType>
class Writer : public TagBase {
public:
  Writer(WriterCallback *cb, WriterType &async_writer,
         size_t buf_size = 1024 * 1024 * 32)
      : callback_(*cb), writer_impl_(async_writer) {
    auto_write_ = true;
    max_buffer_size_ = buf_size;
    cur_buffer_size_ = 0;
    status_ = STOP;
    input_id = output_id = 0;
  };

  virtual ~Writer() { Stop(); }

  void SetAuto(bool auto_write) { auto_write_ = auto_write; }

  void Start() {
    lock_t lock(mtx_);
    if (status_ == STOP) {
      status_ = IDLE;
    }
  }

  void Stop() {
    lock_t lock(mtx_);
    status_ = STOP;
    while (write_buffer_.size() > 0) {
      RequestType *w = write_buffer_.front();
      write_buffer_.pop_front();
      google::protobuf::Arena *arena = w->GetArena();
      if (arena) {
        delete arena;
      }
    }
    cur_buffer_size_ = 0;
  }

  int Write(const RequestType &request) {
    lock_t lock(mtx_);
    if (status_ == STOP) {
      return -1;
    }

    if (cur_buffer_size_ >= max_buffer_size_) {
      return -1;
    }

    google::protobuf::Arena *arena = new google::protobuf::Arena();
    RequestType *w = google::protobuf::Arena::CreateMessage<RequestType>(arena);
    *w = request;
    write_buffer_.push_back(w);
    cur_buffer_size_ += arena->SpaceUsed();

    if (status_ == IDLE) {
      GPR_ASSERT(write_buffer_.size() == 1);
      status_ = WRITING;
      writer_impl_.Write(*write_buffer_.front(), this);
    }

    return input_id++;
  }

  template <class _InputIter>
  std::pair<int, int> Write(_InputIter __first, _InputIter __last) {
    lock_t lock(mtx_);

    if (status_ == STOP) {
      return {-1, -1};
    }

    if (__first == __last) {
      return {-1, -1};
    }

    if (cur_buffer_size_ >= max_buffer_size_) {
      return {-1, -1};
    }

    for (_InputIter it = __first; it != __last; ++it) {
      google::protobuf::Arena *arena = new google::protobuf::Arena();
      RequestType *w =
          google::protobuf::Arena::CreateMessage<RequestType>(arena);
      *w = *it;
      write_buffer_.push_back(w);
      cur_buffer_size_ += arena->SpaceUsed();
    }

    if (status_ == IDLE) {
      GPR_ASSERT(write_buffer_.size() == std::distance(__first, __last));
      status_ = WRITING;
      writer_impl_.Write(*write_buffer_.front(), this);
    }

    int original = input_id;
    input_id += std::distance(__first, __last);
    return {original, input_id - 1};
  }

  void WriteNext() {
    if (status_ == STOP || status_ == IDLE) {
      return;
    }
    GPR_ASSERT(write_buffer_.size() > 0);
    writer_impl_.Write(*write_buffer_.front(), this);
  }

  void Finish(const grpc::Status &status, void *tag) {
    writer_impl_.Finish(status, tag);
  }

  virtual void Process() {
    lock_t lock(mtx_);
    if (status_ == STOP) {
      return;
    }
    GPR_ASSERT(write_buffer_.size() >= 1);
    RequestType *w = write_buffer_.front();
    write_buffer_.pop_front();
    google::protobuf::Arena *arena = w->GetArena();
    if (arena) {
      cur_buffer_size_ -= arena->SpaceUsed();
      delete arena;
    }
    callback_.OnWrite(output_id++);
    if (write_buffer_.size() > 0) {
      if (auto_write_) {
        writer_impl_.Write(*write_buffer_.front(), this);
      }
    } else {
      status_ = IDLE;
    }
  };

  virtual void OnError() { callback_.OnWriteError(); }

private:
  enum CallStatus { IDLE, WRITING, STOP };
  CallStatus status_;

  typedef std::unique_lock<std::mutex> lock_t;
  std::mutex mtx_;

  std::list<RequestType *> write_buffer_;
  size_t max_buffer_size_;
  size_t cur_buffer_size_;

  WriterCallback &callback_;
  WriterType &writer_impl_;

  int input_id;
  int output_id;

  bool auto_write_;
};

} // namespace grpc_framework

#endif // GRPC_FRAMEWORK_RPC_WRITER_H

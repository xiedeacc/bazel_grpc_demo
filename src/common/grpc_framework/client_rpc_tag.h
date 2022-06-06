//
// Created by zjf on 2018/3/9.
//

#ifndef GRPC_FRAMEWORK_CLIENT_RPC_H
#define GRPC_FRAMEWORK_CLIENT_RPC_H

#include "src/common/grpc_framework/client_impl.h"
#include "src/common/grpc_framework/client_rpc_reader.h"
#include "src/common/grpc_framework/client_rpc_writer.h"
#include "src/common/grpc_framework/tag_base.h"

#include <grpcpp/grpcpp.h>

namespace grpc_demo {
namespace common {
namespace grpc_framework {

enum class ClientRPCStatus {
  CREATE,
  READ,
  WRITE,
  WORKING,
  FINISH,
  DESTORY,
  ERR
};

template <typename RequestType, typename ResponseType>
class ClientUnaryStreamRpcTag : public TagBase, public ReaderCallback {
public:
  ClientUnaryStreamRpcTag() { status = ClientRPCStatus::CREATE; };

  virtual ~ClientUnaryStreamRpcTag() {}

  virtual std::string Name() override { return "ClientUnaryStreamRpcTag"; }

  virtual void Process() override = 0;

  virtual void Finish() {
    status = grpc_demo::common::grpc_framework::ClientRPCStatus::FINISH;
    stream->Finish(&rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
  };

  virtual void OnError() override {
    status = ClientRPCStatus::FINISH;
    stream->Finish(&rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
    // below code better perfomance, but not so formal
    // context.TryCancel();
    // this->Process();
  };

  virtual void OnRead(void *message) override = 0;

  virtual void OnReadError() override { OnError(); };

protected:
  // client_server_impl<SERVICE>* client;
  grpc::ClientContext context;
  ClientRPCStatus status;
  grpc::Status rpc_status;

  std::unique_ptr<grpc::ClientAsyncReader<ResponseType>> stream;
  typedef Reader<ResponseType, grpc::ClientAsyncReader<ResponseType>>
      ReaderType;
  std::unique_ptr<ReaderType> reader_;

  RequestType request;
};

template <typename RequestType, typename ResponseType>
class ClientBiStreamRpcTag : public TagBase,
                             public ReaderCallback,
                             public WriterCallback {
public:
  ClientBiStreamRpcTag() { status = ClientRPCStatus::CREATE; };

  virtual ~ClientBiStreamRpcTag() {}

  virtual std::string Name() override { return "ClientBiStreamRpcTag"; }

  virtual void Process() override = 0;

  virtual void OnError() override {
    status = ClientRPCStatus::FINISH;
    // stream->Finish(&rpc_status, this);
    // LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
    //<< ", message: " << rpc_status.error_message().c_str();
    // below code better perfomance, but not so formal
    context.TryCancel();
    this->Process();
  };

  virtual void OnRead(void *req_ptr) override = 0;

  virtual void OnReadError() override = 0;

  virtual void OnWrite(int write_id) override = 0;

  virtual void OnWriteError() override {
    writer_->Stop();
    OnError();
  };

  virtual void Finish() {
    status = grpc_demo::common::grpc_framework::ClientRPCStatus::FINISH;
    stream->Finish(&rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
  };

protected:
  grpc::ClientContext context;
  ClientRPCStatus status;
  grpc::Status rpc_status;

  std::unique_ptr<grpc::ClientAsyncReaderWriter<RequestType, ResponseType>>
      stream;
  typedef Reader<ResponseType,
                 grpc::ClientAsyncReaderWriter<RequestType, ResponseType>>
      ReaderType;

  typedef Writer<RequestType,
                 grpc::ClientAsyncReaderWriter<RequestType, ResponseType>>
      WriterType;
  std::unique_ptr<ReaderType> reader_;
  std::unique_ptr<WriterType> writer_;
};

} // namespace grpc_framework
} // namespace common
} // namespace grpc_demo
#endif // GRPC_FRAMEWORK_CLIENT_RPC_H

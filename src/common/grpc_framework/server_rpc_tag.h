//
// Created by zjf on 2018/3/9.
//

#ifndef GRPC_FRAMEWORK_SERVER_RPC_H
#define GRPC_FRAMEWORK_SERVER_RPC_H

#include "src/common/grpc_framework/server_impl.h"
#include "src/common/grpc_framework/server_rpc_reader.h"
#include "src/common/grpc_framework/server_rpc_writer.h"
#include "src/common/grpc_framework/tag_base.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/support/status.h>

namespace grpc_demo {
namespace common {
namespace grpc_framework {

enum class ServerRPCStatus {
  CREATE,
  READ,
  WRITE,
  WORKING,
  FINISH,
  DESTORY,
  ERR
};

template <typename RequestType, typename ResponseType>
class ServerUnaryStreamRpcTag : public TagBase, public WriterCallback {
public:
  ServerUnaryStreamRpcTag() { status = ServerRPCStatus::CREATE; };

  virtual ~ServerUnaryStreamRpcTag() {}

  virtual std::string Name() override { return "ServerUnaryStreamRpcTag"; }

  virtual void Process() override = 0;

  virtual void Finish() override {
    status = grpc_demo::common::grpc_framework::ServerRPCStatus::FINISH;
    responder_.Finish(rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
  };

  virtual void OnError() override {
    status = ServerRPCStatus::FINISH;
    responder_.Finish(rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
    // below code better perfomance, but not so formal
    // server_context_.TryCancel();
    // this->Process();
  };

  virtual void OnWrite(void *message) override = 0;

  virtual void OnWriteError() override = 0;

protected:
  grpc::ServerContext server_context_;
  ServerRPCStatus status;
  grpc::Status rpc_status;

  typename grpc::ServerAsyncWriter<ResponseType> responder_;
  typedef Writer<ResponseType, grpc::ServerAsyncWriter<ResponseType>>
      WriterType;
  std::unique_ptr<WriterType> writer_;
  ResponseType response;
};

template <typename RequestType, typename ResponseType>
class ServerBiStreamRpcTag : public TagBase,
                             public ReaderCallback,
                             public WriterCallback {
public:
  ServerBiStreamRpcTag() : responder_(&server_context_) {
    status = ServerRPCStatus::CREATE;
  };

  virtual ~ServerBiStreamRpcTag() {}

  virtual std::string Name() override { return "ServerBiStreamRpcTag"; }

  virtual void Process() override = 0;

  virtual void Finish() {
    status = grpc_demo::common::grpc_framework::ServerRPCStatus::FINISH;
    responder_.Finish(rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
  };

  virtual void OnError() override {
    status = ServerRPCStatus::FINISH;
    responder_.Finish(rpc_status, this);
    LOG(INFO) << "on_error, error_code: " << rpc_status.error_code()
              << ", message: " << rpc_status.error_message().c_str();
    // below code better perfomance, but not so formal
    // server_context_.TryCancel();
    // this->Process();
  };

  virtual void OnRead(void *req_ptr) override = 0;

  virtual void OnReadError() override = 0;

  virtual void OnWrite(int write_id) override = 0;

  virtual void OnWriteError() override {
    writer_->Stop();
    OnError();
  };

protected:
  grpc::ServerContext server_context_;
  ServerRPCStatus status;
  grpc::Status rpc_status;

  typedef Reader<RequestType,
                 grpc::ServerAsyncReaderWriter<ResponseType, RequestType>>
      ReaderType;

  typedef Writer<ResponseType,
                 grpc::ServerAsyncReaderWriter<ResponseType, RequestType>>
      WriterType;

  typename grpc::ServerAsyncReaderWriter<ResponseType, RequestType> responder_;
  std::unique_ptr<ReaderType> reader_;
  std::unique_ptr<WriterType> writer_;
};

} // namespace grpc_framework
} // namespace common
} // namespace grpc_demo
#endif // GRPC_FRAMEWORK_SERVER_RPC_H

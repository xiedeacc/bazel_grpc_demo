//
// Created by zjf on 2018/3/9.
//

#ifndef GRPC_FRAMEWORK_CLIENT_RPC_H
#define GRPC_FRAMEWORK_CLIENT_RPC_H

#include "src/grpc_client/async_stream/grpc_framework/client_impl.h"
#include "src/grpc_client/async_stream/grpc_framework/rpc_reader.h"
#include "src/grpc_client/async_stream/grpc_framework/rpc_writer.h"
#include "src/grpc_client/async_stream/grpc_framework/tag_base.h"

#include "grpc++/grpc++.h"

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

  virtual void Process() override = 0;

  virtual void OnError() override {
    status = ClientRPCStatus::FINISH;
    context.TryCancel();
    //        stream->Finish(&rpc_status, this);
    this->Process();
  };

  virtual void OnRead(void *message) override = 0;

  virtual void OnReadError() override { OnError(); };

protected:
  // client_server_impl<SERVICE>* client;
  grpc::ClientContext context;
  ClientRPCStatus status;

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

  virtual void Process() override = 0;

  virtual void OnError() override {
    status = ClientRPCStatus::FINISH;
    //        stream->Finish(&rpc_status, this);
    context.TryCancel();
    this->Process();
  };

  virtual void OnRead(void *req_ptr) override = 0;

  virtual void OnReadError() override { OnError(); };

  virtual void OnWrite(int write_id) override = 0;

  virtual void OnWriteError() override { OnError(); };

  int Write(const RequestType &request) { return writer_->Write(request); };

protected:
  grpc::ClientContext context;
  ClientRPCStatus status;

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
#endif // GRPC_FRAMEWORK_CLIENT_RPC_H

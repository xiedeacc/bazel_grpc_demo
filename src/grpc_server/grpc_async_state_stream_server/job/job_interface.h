/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_SERVER_JOB_INTERFACE_H
#define GRPC_SERVER_JOB_INTERFACE_H
#pragma once

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace job {

class JobInterface {
public:
  virtual void Proceed(bool ok) = 0;
  virtual void OnDone(bool ok) = 0;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_SERVER_JOB_INTERFACE_H

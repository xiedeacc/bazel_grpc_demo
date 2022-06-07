/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_SERVER_JOB_DONE_HANDLER_H
#define GRPC_SERVER_JOB_DONE_HANDLER_H
#pragma once

#include "src/grpc_server/grpc_async_state_stream_server/job/job_interface.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_state_stream_server {
namespace job {

class JobDoneHandler {
public:
  JobDoneHandler(JobInterface *job) : job_(job) {}
  virtual void Proceed(bool ok) { job_->OnDone(ok); };

private:
  JobInterface *job_;
};

} // namespace job
} // namespace grpc_async_state_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_SERVER_JOB_DONE_HANDLER_H

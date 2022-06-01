/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/common/proto/grpc_service.grpc.pb.h"
#include "src/common/proto/grpc_service.pb.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/base_job.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/bidirectional_streaming_job.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/client_streaming_job.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/server_streaming_job.h"
#include "src/grpc_server/grpc_async_callback_stream_server/job/unary_job.h"

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {
namespace job {

template <>
std::atomic<int32_t> BidirectionalStreamingJob<
    grpc_demo::common::proto::RouteGuide::AsyncService,
    grpc_demo::common::proto::RouteNote,
    grpc_demo::common::proto::RouteNote>::gBidirectionalStreamingJobCounter(0);

template <>
std::atomic<int32_t> ClientStreamingJob<
    grpc_demo::common::proto::RouteGuide::AsyncService,
    grpc_demo::common::proto::Point,
    grpc_demo::common::proto::RouteSummary>::gClientStreamingJobCounter(0);

template <>
std::atomic<int32_t> ServerStreamingJob<
    grpc_demo::common::proto::RouteGuide::AsyncService,
    grpc_demo::common::proto::Rectangle,
    grpc_demo::common::proto::Feature>::gServerStreamingJobCounter(0);

template <>
std::atomic<int32_t>
    UnaryJob<grpc_demo::common::proto::RouteGuide::AsyncService,
             grpc_demo::common::proto::Point,
             grpc_demo::common::proto::Feature>::gUnaryJobCounter(0);

} // namespace job
} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

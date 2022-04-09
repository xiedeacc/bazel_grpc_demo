/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/grpc_server/job/base_job.h"
#include "src/grpc_server/job/bidirectional_streaming_job.h"
#include "src/grpc_server/job/client_streaming_job.h"
#include "src/grpc_server/job/server_streaming_job.h"
#include "src/grpc_server/job/unary_job.h"
#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"

namespace grpc_demo {
namespace grpc_server {
namespace job {

template <>
std::atomic<int32_t> BidirectionalStreamingJob<
    grpc_demo::grpc_server::RouteGuide::AsyncService,
    grpc_demo::grpc_server::RouteNote,
    grpc_demo::grpc_server::RouteNote>::gBidirectionalStreamingJobCounter(0);

template <>
std::atomic<int32_t> ClientStreamingJob<
    grpc_demo::grpc_server::RouteGuide::AsyncService,
    grpc_demo::grpc_server::Point,
    grpc_demo::grpc_server::RouteSummary>::gClientStreamingJobCounter(0);

template <>
std::atomic<int32_t> ServerStreamingJob<
    grpc_demo::grpc_server::RouteGuide::AsyncService,
    grpc_demo::grpc_server::Rectangle,
    grpc_demo::grpc_server::Feature>::gServerStreamingJobCounter(0);

template <>
std::atomic<int32_t>
    UnaryJob<grpc_demo::grpc_server::RouteGuide::AsyncService,
             grpc_demo::grpc_server::Point,
             grpc_demo::grpc_server::Feature>::gUnaryJobCounter(0);

} // namespace job
} // namespace grpc_server
} // namespace grpc_demo

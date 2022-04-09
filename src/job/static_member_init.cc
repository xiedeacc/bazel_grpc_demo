/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include "src/job/base_job.h"
#include "src/job/bidirectional_streaming_job.h"
#include "src/job/client_streaming_job.h"
#include "src/job/server_streaming_job.h"
#include "src/job/unary_job.h"
#include "src/proto/grpc_service.grpc.pb.h"
#include "src/proto/grpc_service.pb.h"

namespace grpc_demo {
namespace job {

template <>
std::atomic<int32_t> BidirectionalStreamingJob<
    grpc_demo::RouteGuide::AsyncService, grpc_demo::RouteNote,
    grpc_demo::RouteNote>::gBidirectionalStreamingJobCounter(0);

template <>
std::atomic<int32_t>
    ClientStreamingJob<grpc_demo::RouteGuide::AsyncService, grpc_demo::Point,
                       grpc_demo::RouteSummary>::gClientStreamingJobCounter(0);

template <>
std::atomic<int32_t>
    ServerStreamingJob<grpc_demo::RouteGuide::AsyncService,
                       grpc_demo::Rectangle,
                       grpc_demo::Feature>::gServerStreamingJobCounter(0);

template <>
std::atomic<int32_t>
    UnaryJob<grpc_demo::RouteGuide::AsyncService, grpc_demo::Point,
             grpc_demo::Feature>::gUnaryJobCounter(0);

} // namespace job
} // namespace grpc_demo

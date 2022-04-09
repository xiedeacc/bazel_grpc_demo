/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_DEMO_UTIL_HELPER_H_
#define GRPC_DEMO_UTIL_HELPER_H_

#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"
#include <string>
#include <vector>

namespace grpc_demo {
namespace grpc_server {
namespace util {

std::string GetDbFileContent(int argc, char **argv);

void ParseDb(const std::string &db,
             std::vector<grpc_demo::grpc_server::Feature> *feature_list);

float ConvertToRadians(float num);

float GetDistance(const grpc_demo::grpc_server::Point &start,
                  const grpc_demo::grpc_server::Point &end);

std::string GetFeatureName(
    const grpc_demo::grpc_server::Point &point,
    const std::vector<grpc_demo::grpc_server::Feature> &feature_list);

} // namespace util
} // namespace grpc_server
} // namespace grpc_demo

#endif // GRPC_DEMO_UTIL_HELPER_H_

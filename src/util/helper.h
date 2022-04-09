/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef GRPC_DEMO_UTIL_HELPER_H_
#define GRPC_DEMO_UTIL_HELPER_H_

#include "src/proto/grpc_service.grpc.pb.h"
#include "src/proto/grpc_service.pb.h"
#include <string>
#include <vector>

namespace grpc_demo {
namespace util {

std::string GetDbFileContent(int argc, char **argv);

void ParseDb(const std::string &db,
             std::vector<grpc_demo::Feature> *feature_list);

float ConvertToRadians(float num);

float GetDistance(const grpc_demo::Point &start, const grpc_demo::Point &end);

std::string GetFeatureName(const grpc_demo::Point &point,
                           const std::vector<grpc_demo::Feature> &feature_list);

} // namespace util
} // namespace grpc_demo

#endif // GRPC_DEMO_UTIL_HELPER_H_

/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#ifndef TAG_INFO_H
#define TAG_INFO_H
#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <vector>

namespace grpc_demo {
namespace grpc_server {
namespace grpc_async_callback_stream_server {

struct TagInfo {
  std::function<void(bool)> *tagProcessor;
  bool ok;
};

} // namespace grpc_async_callback_stream_server
} // namespace grpc_server
} // namespace grpc_demo

#endif // TAG_INFO_H

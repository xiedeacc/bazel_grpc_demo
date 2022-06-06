/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <string>

#include "src/grpc_server/grpc_async_server/route_guide_server_impl.h"

int main(int argc, char **argv) {
  std::string db_content =
      grpc_demo::common::util::GetDbFileContent(argc, argv);
  grpc_demo::grpc_server::grpc_async_server::ServerImpl server(db_content);
  server.Run("[::]:50051");
  return 0;
}

//
// Created by zjf on 2018/2/13.
//

#ifndef GRPC_FRAMEWORK_TAGBASE_H
#define GRPC_FRAMEWORK_TAGBASE_H

#include "glog/logging.h"
namespace grpc_framework {
class TagBase {
public:
  virtual void Process() = 0;

  virtual void OnError() = 0;
};

} // namespace grpc_framework
#endif // GRPC_FRAMEWORK_TAGBASE_H

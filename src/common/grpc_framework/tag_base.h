//
// Created by zjf on 2018/2/13.
//

#ifndef GRPC_FRAMEWORK_TAGBASE_H
#define GRPC_FRAMEWORK_TAGBASE_H

namespace grpc_demo {
namespace common {
namespace grpc_framework {
class TagBase {
public:
  virtual void Process() = 0;

  virtual void OnError() = 0;
};

} // namespace grpc_framework
} // namespace common
} // namespace grpc_demo

#endif // GRPC_FRAMEWORK_TAGBASE_H

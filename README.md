# Introduce

bazel_grpc_demo is the best example for grpc in github for current, it can be use in product enviroment. 

# Feature

1. supprot bazel 5.1 and above
2. many implement style, state machine style, callback, unary and bidirectional and client and server streaming, synchronous
3. multiple thread
4. high performance

# Todo:

1. grpc_async_server使用arena方式分配内存会导致core，直接new不会core，原因待查
2. grpc_async_server没有调用AsyncNotifyWhenDone，调用AsyncNotifyWhenDone需要像grpc_async_callback_stream_server和grpc_async_state_stream_server一样记录是否还有tagzai队列中


# About code
1. some code derrived from https://groups.google.com/g/grpc-io/c/T9u2TejYVTc/m/bgtMIKpfCgAJ, 
2. some code derrived from https://github.com/tnie/GrpcExamples/tree/master/cpp/async_stream, it's really a sophisticated example.
3. thanks to Arpit Baldeva and tnie

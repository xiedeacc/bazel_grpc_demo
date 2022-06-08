# Introduce

bazel_grpc_demo is the best example for grpc in github for current, it can be use in product enviroment. 

# Feature

1. supprot bazel 5.1 and above
2. many implement style, state machine style, callback, unary and bidirectional and client and server streaming, synchronous
3. multiple thread
4. high performance

# Todo:

1. grpc_async_server use grpc arena to allocate and construct response, then modify response will core，new reponse works fine, need check reason.
2. grpc_async_server not call AsyncNotifyWhenDone，call AsyncNotifyWhenDone need make records and do some judgement like grpc_async_callback_stream_server and grpc_async_state_stream_server

# About code
1. some code derrived from https://groups.google.com/g/grpc-io/c/T9u2TejYVTc/m/bgtMIKpfCgAJ, 
2. some code derrived from https://github.com/tnie/GrpcExamples/tree/master/cpp/async_stream, it's really a sophisticated example.
3. thanks to Arpit Baldeva and tnie

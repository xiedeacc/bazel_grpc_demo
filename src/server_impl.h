
float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

float GetDistance(const routeguide::Point &start,
                  const routeguide::Point &end) {
  const float kCoordFactor = 10000000.0;
  float lat_1 = start.latitude() / kCoordFactor;
  float lat_2 = end.latitude() / kCoordFactor;
  float lon_1 = start.longitude() / kCoordFactor;
  float lon_2 = end.longitude() / kCoordFactor;
  float lat_rad_1 = ConvertToRadians(lat_1);
  float lat_rad_2 = ConvertToRadians(lat_2);
  float delta_lat_rad = ConvertToRadians(lat_2 - lat_1);
  float delta_lon_rad = ConvertToRadians(lon_2 - lon_1);

  float a = pow(sin(delta_lat_rad / 2), 2) +
            cos(lat_rad_1) * cos(lat_rad_2) * pow(sin(delta_lon_rad / 2), 2);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  int R = 6371000; // metres

  return R * c;
}

std::string
GetFeatureName(const routeguide::Point &point,
               const std::vector<routeguide::Feature> &feature_list) {
  for (const routeguide::Feature &f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

class ServerImpl final {
public:
  ServerImpl() { routeguide::ParseDb(gDB, &mFeatureList); }

  ~ServerImpl() {
    mServer->Shutdown();
    // Always shutdown the completion queue after the server.
    mCQ->Shutdown();
  }

  // There is no shutdown handling in this code.
  void Run() {
    std::string server_address("0.0.0.0:50051");

    grpc::ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service_" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *asynchronous* service.
    builder.RegisterService(&mRouteGuideService);
    // Get hold of the completion queue used for the asynchronous communication
    // with the gRPC runtime.
    mCQ = builder.AddCompletionQueue();
    // Finally assemble the server.
    mServer = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;

    // Proceed to the server's main loop.
    HandleRpcs();
  }

private:
  // Handlers for various rpcs. An application could do custom code generation
  // for creating these (except the actual processing logic).
  void createGetFeatureRpc() {
    UnaryRpcHandlers<routeguide::RouteGuide::AsyncService, routeguide::Point,
                     routeguide::Feature>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createGetFeatureRpc, this);

    rpcHandlers.processIncomingRequest = &GetFeatureProcessor;
    rpcHandlers.done = &GetFeatureDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestGetFeature;

    new UnaryRpc<routeguide::RouteGuide::AsyncService, routeguide::Point,
                 routeguide::Feature>(&mRouteGuideService, mCQ.get(),
                                      rpcHandlers);
  }

  static void GetFeatureProcessor(RpcBase &rpc,
                                  const google::protobuf::Message *message) {
    auto point = static_cast<const routeguide::Point *>(message);

    routeguide::Feature feature;
    feature.set_name(GetFeatureName(*point, gServerImpl->mFeatureList));
    feature.mutable_location()->CopyFrom(*point);

    randomSleepThisThread();
    rpc.sendResponse(&feature);
  }

  static void GetFeatureDone(RpcBase &rpc, bool rpcCancelled) { delete (&rpc); }

  void createListFeaturesRpc() {
    ServerStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                               routeguide::Rectangle, routeguide::Feature>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createListFeaturesRpc, this);

    rpcHandlers.processIncomingRequest = &ListFeaturesProcessor;
    rpcHandlers.done = &ListFeaturesDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestListFeatures;

    new ServerStreamingRpc<routeguide::RouteGuide::AsyncService,
                           routeguide::Rectangle, routeguide::Feature>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  static void ListFeaturesProcessor(RpcBase &rpc,
                                    const google::protobuf::Message *message) {
    auto rectangle = static_cast<const routeguide::Rectangle *>(message);

    auto lo = rectangle->lo();
    auto hi = rectangle->hi();
    long left = (std::min)(lo.longitude(), hi.longitude());
    long right = (std::max)(lo.longitude(), hi.longitude());
    long top = (std::max)(lo.latitude(), hi.latitude());
    long bottom = (std::min)(lo.latitude(), hi.latitude());
    for (auto f : gServerImpl->mFeatureList) {
      if (f.location().longitude() >= left &&
          f.location().longitude() <= right &&
          f.location().latitude() >= bottom && f.location().latitude() <= top) {
        rpc.sendResponse(&f);
        randomSleepThisThread();
      }
    }
    rpc.sendResponse(nullptr);
  }

  static void ListFeaturesDone(RpcBase &rpc, bool rpcCancelled) {
    delete (&rpc);
  }

  void createRecordRouteRpc() {
    ClientStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                               routeguide::Point, routeguide::RouteSummary>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createRecordRouteRpc, this);

    rpcHandlers.processIncomingRequest = &RecordRouteProcessor;
    rpcHandlers.done = &RecordRouteDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestRecordRoute;

    new ClientStreamingRpc<routeguide::RouteGuide::AsyncService,
                           routeguide::Point, routeguide::RouteSummary>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  struct RecordRouteState {
    int pointCount;
    int featureCount;
    float distance;
    routeguide::Point previous;
    std::chrono::system_clock::time_point startTime;
    RecordRouteState() : pointCount(0), featureCount(0), distance(0.0f) {}
  };

  std::unordered_map<RpcBase *, RecordRouteState> mRecordRouteMap;
  static void RecordRouteProcessor(RpcBase &rpc,
                                   const google::protobuf::Message *message) {
    auto point = static_cast<const routeguide::Point *>(message);

    RecordRouteState &state = gServerImpl->mRecordRouteMap[&rpc];

    if (point) {
      if (state.pointCount == 0)
        state.startTime = std::chrono::system_clock::now();

      state.pointCount++;
      if (!GetFeatureName(*point, gServerImpl->mFeatureList).empty()) {
        state.featureCount++;
      }
      if (state.pointCount != 1) {
        state.distance += GetDistance(state.previous, *point);
      }
      state.previous = *point;

      randomSleepThisThread();
    } else {
      std::chrono::system_clock::time_point endTime =
          std::chrono::system_clock::now();

      routeguide::RouteSummary summary;
      summary.set_point_count(state.pointCount);
      summary.set_feature_count(state.featureCount);
      summary.set_distance(static_cast<long>(state.distance));
      auto secs = std::chrono::duration_cast<std::chrono::seconds>(
          endTime - state.startTime);
      summary.set_elapsed_time(secs.count());
      rpc.sendResponse(&summary);

      gServerImpl->mRecordRouteMap.erase(&rpc);
      randomSleepThisThread();
    }
  }

  static void RecordRouteDone(RpcBase &rpc, bool rpcCancelled) {
    delete (&rpc);
  }

  void createRouteChatRpc() {
    BidirectionalStreamingRpcHandlers<routeguide::RouteGuide::AsyncService,
                                      routeguide::RouteNote,
                                      routeguide::RouteNote>
        rpcHandlers;

    rpcHandlers.createRpc = std::bind(&ServerImpl::createRouteChatRpc, this);

    rpcHandlers.processIncomingRequest = &RouteChatProcessor;
    rpcHandlers.done = &RouteChatDone;

    rpcHandlers.requestRpc =
        &routeguide::RouteGuide::AsyncService::RequestRouteChat;

    new BidirectionalStreamingRpc<routeguide::RouteGuide::AsyncService,
                                  routeguide::RouteNote, routeguide::RouteNote>(
        &mRouteGuideService, mCQ.get(), rpcHandlers);
  }

  static void RouteChatProcessor(RpcBase &rpc,
                                 const google::protobuf::Message *message) {
    auto note = static_cast<const routeguide::RouteNote *>(message);
    // Simply echo the note back.
    if (note) {
      routeguide::RouteNote responseNote(*note);
      rpc.sendResponse(&responseNote);
      randomSleepThisThread();
    } else {
      rpc.sendResponse(nullptr);
      randomSleepThisThread();
    }
  }

  static void RouteChatDone(RpcBase &rpc, bool rpcCancelled) { delete (&rpc); }

  void HandleRpcs() {
    createGetFeatureRpc();
    createListFeaturesRpc();
    createRecordRouteRpc();
    createRouteChatRpc();

    TagInfo tagInfo;
    while (true) {
      // Block waiting to read the next event from the completion queue. The
      // event is uniquely identified by its tag, which in this case is the
      // memory address of a CallData instance.
      // The return value of Next should always be checked. This return value
      // tells us whether there is any kind of event or cq_ is shutting down.
      GPR_ASSERT(mCQ->Next((void **)&tagInfo.tagProcessor,
                           &tagInfo.ok)); // GRPC_TODO - Handle returned value

      gIncomingTagsMutex.lock();
      gIncomingTags.push_back(tagInfo);
      gIncomingTagsMutex.unlock();
    }
  }

  std::unique_ptr<grpc::ServerCompletionQueue> mCQ;
  routeguide::RouteGuide::AsyncService mRouteGuideService;
  std::vector<routeguide::Feature> mFeatureList;
  std::unique_ptr<grpc::Server> mServer;
};

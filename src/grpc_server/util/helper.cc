/*******************************************************************************
 * Copyright (c) 2022 Copyright 2022- xiedeacc.com.
 * All rights reserved.
 *******************************************************************************/

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "src/grpc_server/proto/grpc_service.grpc.pb.h"
#include "src/grpc_server/proto/grpc_service.pb.h"

namespace grpc_demo {
namespace grpc_server {
namespace util {

using grpc_demo::grpc_server::Feature;

std::string GetDbFileContent(int argc, char **argv) {
  std::string db_path;
  std::string arg_str("--db_path");
  if (argc > 1) {
    std::string argv_1 = argv[1];
    size_t start_position = argv_1.find(arg_str);
    if (start_position != std::string::npos) {
      start_position += arg_str.size();
      if (argv_1[start_position] == ' ' || argv_1[start_position] == '=') {
        db_path = argv_1.substr(start_position + 1);
      }
    }
  } else {
    db_path = "/root/tmp/db.json";
  }
  std::ifstream db_file(db_path);
  if (!db_file.is_open()) {
    std::cout << "Failed to open " << db_path << std::endl;
    return "";
  }
  std::stringstream db;
  db << db_file.rdbuf();
  return db.str();
}

class Parser {
public:
  explicit Parser(const std::string &db) : db_(db) {
    // Remove all spaces.
    db_.erase(std::remove_if(db_.begin(), db_.end(), isspace), db_.end());
    if (!Match("[")) {
      SetFailedAndReturnFalse();
    }
  }

  bool Finished() { return current_ >= db_.size(); }

  bool TryParseOne(Feature *feature) {
    if (failed_ || Finished() || !Match("{")) {
      return SetFailedAndReturnFalse();
    }
    if (!Match(location_) || !Match("{") || !Match(latitude_)) {
      return SetFailedAndReturnFalse();
    }
    long temp = 0;
    ReadLong(&temp);
    feature->mutable_location()->set_latitude(temp);
    if (!Match(",") || !Match(longitude_)) {
      return SetFailedAndReturnFalse();
    }
    ReadLong(&temp);
    feature->mutable_location()->set_longitude(temp);
    if (!Match("},") || !Match(name_) || !Match("\"")) {
      return SetFailedAndReturnFalse();
    }
    size_t name_start = current_;
    while (current_ != db_.size() && db_[current_++] != '"') {
    }
    if (current_ == db_.size()) {
      return SetFailedAndReturnFalse();
    }
    feature->set_name(db_.substr(name_start, current_ - name_start - 1));
    if (!Match("},")) {
      if (db_[current_ - 1] == ']' && current_ == db_.size()) {
        return true;
      }
      return SetFailedAndReturnFalse();
    }
    return true;
  }

private:
  bool SetFailedAndReturnFalse() {
    failed_ = true;
    return false;
  }

  bool Match(const std::string &prefix) {
    bool eq = db_.substr(current_, prefix.size()) == prefix;
    current_ += prefix.size();
    return eq;
  }

  void ReadLong(long *l) {
    size_t start = current_;
    while (current_ != db_.size() && db_[current_] != ',' &&
           db_[current_] != '}') {
      current_++;
    }
    // It will throw an exception if fails.
    *l = std::stol(db_.substr(start, current_ - start));
  }

  bool failed_ = false;
  std::string db_;
  size_t current_ = 0;
  const std::string location_ = "\"location\":";
  const std::string latitude_ = "\"latitude\":";
  const std::string longitude_ = "\"longitude\":";
  const std::string name_ = "\"name\":";
};

void ParseDb(const std::string &db, std::vector<Feature> *feature_list) {
  feature_list->clear();
  std::string db_content(db);
  db_content.erase(
      std::remove_if(db_content.begin(), db_content.end(), isspace),
      db_content.end());

  Parser parser(db_content);
  Feature feature;
  while (!parser.Finished()) {
    feature_list->push_back(Feature());
    if (!parser.TryParseOne(&feature_list->back())) {
      std::cout << "Error parsing the db file";
      feature_list->clear();
      break;
    }
  }
  std::cout << "DB parsed, loaded " << feature_list->size() << " features."
            << std::endl;
}

float ConvertToRadians(float num) { return num * 3.1415926 / 180; }

float GetDistance(const grpc_demo::grpc_server::Point &start,
                  const grpc_demo::grpc_server::Point &end) {
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

std::string GetFeatureName(
    const grpc_demo::grpc_server::Point &point,
    const std::vector<grpc_demo::grpc_server::Feature> &feature_list) {
  for (const grpc_demo::grpc_server::Feature &f : feature_list) {
    if (f.location().latitude() == point.latitude() &&
        f.location().longitude() == point.longitude()) {
      return f.name();
    }
  }
  return "";
}

} // namespace util
} // namespace grpc_server
} // namespace grpc_demo

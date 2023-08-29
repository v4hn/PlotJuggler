#pragma once

#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace mcap_api
{

using Timestamp = uint64_t;  // nanoseconds since epoch
constexpr Timestamp MaxTime = std::numeric_limits<Timestamp>::max();
using KeyValueMap = std::unordered_map<std::string, std::string>;
using ByteArray = std::vector<std::byte>;
using ChannelId = uint16_t;

struct MessageView
{
  uint16_t channel_id = 0;
  uint64_t timestamp = 0;
  const std::byte* data = nullptr;
  uint32_t data_size = 0;
};

struct ChunkView
{
  int32_t index = -1;
  std::vector<MessageView> msgs;
};

struct ChannelInfo
{
  ChannelId id;
  std::string schema_name;
  std::string schema_encoding;
  // could have non-textual bytes
  std::string schema_data;
  std::string topic;
  std::string message_encoding;
  uint64_t message_count;
  Timestamp start_time;
  Timestamp end_time;
};

struct Statistics
{
  std::string library;
  std::string profile;
  uint64_t message_count = 0;
  Timestamp start_time;
  Timestamp end_time;
  std::vector<ChannelInfo> channels;
};

struct FileInfo
{
  std::string filename;
  std::string dateTime;
  double size_MB;
};

struct DataRequest
{
  std::string filename;
  std::vector<std::string> topics;
  mcap_api::Timestamp start_time = 0;
  mcap_api::Timestamp end_time = MaxTime;
};

}  // namespace mcap_api

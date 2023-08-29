#include "mcap_api/mcap_protocol.h"

namespace mcap_protocol
{
nlohmann::json toJson(const mcap_api::FileInfo& info)
{
  return nlohmann::json{ { "filename", info.filename },
                         { "size", info.size_MB },
                         { "dateTime", info.dateTime } };
}

nlohmann::json toJson(const mcap_api::Statistics& statistics)
{
  auto json = nlohmann::json{ { "library", statistics.library },
                              { "profile", statistics.profile },
                              { "message_count", statistics.message_count },
                              { "start_time", statistics.start_time },
                              { "end_time", statistics.end_time } };

  nlohmann::json jsonArray = nlohmann::json::array();
  for (const auto& channel : statistics.channels)
  {
    auto json = toJson(channel);
    jsonArray.push_back(json);
  }

  json["channels"] = jsonArray;

  return json;
}

nlohmann::json toJson(const mcap_api::ChannelInfo& channel)
{
  return nlohmann::json{ { "channel_id", channel.id },
                         { "schema_name", channel.schema_name },
                         { "schema_encoding", channel.schema_encoding },
                         { "schema_data", channel.schema_data },
                         { "topic", channel.topic },
                         { "message_encoding", channel.message_encoding },
                         { "message_count", channel.message_count },
                         { "start_time", channel.start_time },
                         { "end_time", channel.end_time } };
}

nlohmann::json toJson(const mcap_api::DataRequest& request)
{
  return nlohmann::json{ { "filename", request.filename },
                         { "topics", request.topics },
                         { "start_time", request.start_time },
                         { "end_time", request.end_time } };
}

void fromJson(mcap_api::FileInfo& info, const nlohmann::json& json)
{
  info.filename = json.at("filename").get<std::string>();
  json.at("size").get_to(info.size_MB);
  json.at("dateTime").get_to(info.dateTime);
}

void fromJson(mcap_api::Statistics& statistics, const nlohmann::json& json)
{
  statistics.library = json.at("library").get<std::string>();
  statistics.profile = json.at("profile").get<std::string>();
  json.at("message_count").get_to(statistics.message_count);
  json.at("start_time").get_to(statistics.start_time);
  json.at("end_time").get_to(statistics.end_time);

  auto channels = json.at("channels");
  if (channels.is_array())
  {
    for (const auto& channelJson : channels)
    {
      mcap_api::ChannelInfo channel;
      fromJson(channel, channelJson);
      statistics.channels.push_back(channel);
    }
  }
}

void fromJson(mcap_api::DataRequest& request, const nlohmann::json& json)
{
  json.at("filename").get_to(request.filename);
  json.at("topics").get_to(request.topics);
  json.at("start_time").get_to(request.start_time);
  json.at("end_time").get_to(request.end_time);
}

void fromJson(mcap_api::ChannelInfo& channel, const nlohmann::json& json)
{
  json.at("channel_id").get_to(channel.id);
  channel.schema_name = json.at("schema_name").get<std::string>();
  channel.schema_encoding = json.at("schema_encoding").get<std::string>();
  json.at("schema_data").get_to(channel.schema_data);
  channel.topic = json.at("topic").get<std::string>();
  channel.message_encoding = json.at("message_encoding").get<std::string>();
  json.at("message_count").get_to(channel.message_count);
  json.at("start_time").get_to(channel.start_time);
  json.at("end_time").get_to(channel.end_time);
}


}  // namespace mcap_protocol

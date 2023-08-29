#pragma once

#include "nlohmann/json.hpp"
#include "mcap_api/mcap_api.h"
#include <QByteArray>

namespace mcap_protocol
{

constexpr const char* REQUEST_FILE_LIST = "getFileList";
constexpr const char* REQUEST_STATISTICS = "getStatistics";
constexpr const char* DOWNLOAD_DATA = "downloadData";
constexpr const char* CANCEL_DOWNLOAD = "cancelDownload";
constexpr const char* TRANSFER_STARTED = "transferStarted";
constexpr const char* TRANSFER_COMPLETED = "transferCompleted";
constexpr const char* ERROR_MSG = "errorMessage";

nlohmann::json toJson(const mcap_api::FileInfo& info);
nlohmann::json toJson(const mcap_api::Statistics& statistics);
nlohmann::json toJson(const mcap_api::ChannelInfo& channel);
nlohmann::json toJson(const mcap_api::DataRequest& request);

void fromJson(mcap_api::FileInfo& info, const nlohmann::json& json);
void fromJson(mcap_api::Statistics& statistics, const nlohmann::json& json);
void fromJson(mcap_api::ChannelInfo& channel, const nlohmann::json& json);
void fromJson(mcap_api::DataRequest& request, const nlohmann::json& json);

template <typename T>
inline unsigned Serialize(char* buffer, unsigned offset, const T& value)
{
  memcpy(buffer + offset, &value, sizeof(T));
  return sizeof(T);
}

template <typename T>
inline unsigned Deserialize(const char* buffer, unsigned offset, T& value)
{
  memcpy(&value, buffer + offset, sizeof(T));
  return sizeof(T);
}

}  // namespace mcap_protocol

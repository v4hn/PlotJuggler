#include "mcap_api/mcap_protocol.h"
#include "mcap_files/mcap_files.h"

#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>

#define MCAP_IMPLEMENTATION
#include <mcap/reader.hpp>
#include <mcap/writer.hpp>

namespace mcap_files
{
QStringList GetMCAPFiles(const QString& dirpath)
{
  QStringList filepaths;

  // Use QDirIterator to recursively find files with the .mcap extension
  QDirIterator it(dirpath, QStringList() << "*.mcap", QDir::Files,
                  QDirIterator::Subdirectories);
  while (it.hasNext())
  {
    QString filepath = it.next();
    filepaths.append(filepath);
  }
  return filepaths;
}

mcap_api::FileInfo GetFileInfo(const QString& filepath)
{
  mcap_api::FileInfo basic;
  QFileInfo info(filepath);
  basic.filename = info.fileName().toStdString();
  basic.filepath = filepath.toStdString();
  basic.size_MB = static_cast<double>(info.size()) / (1024 * 1024);
  basic.dateTime = info.lastModified().toString().toStdString();
  return basic;
}

mcap_api::Statistics ReadFileStatistics(mcap::McapReader *reader)
{
  mcap_api::Statistics info;

  auto status = reader->readSummary(mcap::ReadSummaryMethod::NoFallbackScan);
  if (!status.ok())
  {
    std::cerr << "reading summary was not successful."
              << " code: " << int(status.code) << " , message: " << status.message
              << std::endl;
    return info;
  }

  std::cout << "readSummary done" << std::endl;

  info.library = reader->header()->library;
  info.profile = reader->header()->profile;

  auto statisticsOpt = reader->statistics();

  if (statisticsOpt.has_value())
  {
    auto statisticsPtr = statisticsOpt.value();
    info.message_count = statisticsPtr.messageCount;
    info.start_time = statisticsPtr.messageStartTime;
    info.end_time = statisticsPtr.messageEndTime;
    auto schemas = reader->schemas();
    auto channels = reader->channels();
    for (const auto& channelPair : channels)
    {
      const auto& channel = channelPair.second;
      auto channelId = channelPair.first;
      mcap_api::ChannelInfo channelInfo;
      channelInfo.id = channelId;
      channelInfo.topic = channel->topic;
      channelInfo.message_encoding = channel->messageEncoding;
      channelInfo.message_count = statisticsPtr.channelMessageCounts[channelId];
      // can be calculated after reading messages so we may remove these from data
      // structure
      channelInfo.start_time = statisticsPtr.messageStartTime;
      channelInfo.end_time = statisticsPtr.messageEndTime;

      auto schemaIt = schemas.find(channel->schemaId);
      if (schemaIt != schemas.end())
      {
        auto schema = schemaIt->second;
        channelInfo.schema_name = schema->name;
        channelInfo.schema_encoding = schema->encoding;
        channelInfo.schema_data.assign(reinterpret_cast<const char*>(schema->data.data()), schema->data.size());
      }

      for (const auto& pair : channel->metadata)
      {
        std::cout << "channel " << channelId << ", metadata: "
                  << "key " << pair.first << ", value " << pair.second << std::endl;
      }

      info.channels.push_back(channelInfo);
    }
  }
  std::cout << "completed" << std::endl;
  return info;
}


}  // namespace mcap_files

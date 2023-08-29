#pragma once

#include <QDateTime>
#include <QString>

#include <mcap_api/mcap_api.h>

namespace mcap {
class McapReader;
}

namespace mcap_files
{
// Get a list of MCAP files in a certain directory
QStringList GetMCAPFiles(const QString& dirpath);

// Read file info about a particular file
mcap_api::FileInfo GetFileInfo(const QString& filepath);

mcap_api::Statistics ReadFileStatistics(mcap::McapReader* reader);


//bool WriteToMCAP(
//    const std::string& outputFilename, const mcap_api::ChunkArray& chunks,
//    std::unordered_map<mcap_api::ChannelId, mcap_api::ChannelInfo>& channels);

}  // namespace mcap_files


#include <QDebug>
#include <QMessageBox>
#include "mcap_client/mcap_client.h"
#include <iostream>

namespace mcap_client
{

mcap_api::ChunkView deserializeChunk(const QByteArray& buffer)
{
  using namespace mcap_protocol;

  mcap_api::ChunkView chunk;

  int const header_size = sizeof("dataChunk");
  if(buffer.size() < 4 + header_size)
  {
    throw std::runtime_error("Buffer corrupted");
  }

  unsigned offset = header_size;
  offset += Deserialize(buffer.data(), offset, chunk.index);

  const unsigned max_size = unsigned(buffer.size());

  while (offset < max_size)
  {
    if(offset + 14 > max_size)
    {
      throw std::runtime_error("Buffer corrupted");
    }
    mcap_api::MessageView message;
    offset += Deserialize(buffer.data(), offset, message.channel_id);
    offset += Deserialize(buffer.data(), offset, message.timestamp);
    offset += Deserialize(buffer.data(), offset, message.data_size);

    if(offset + message.data_size > max_size)
    {
      throw std::runtime_error("Buffer corrupted");
    }

            // pointer to memory in buffer
    message.data = reinterpret_cast<const std::byte*>(buffer.data() + offset);
    offset += message.data_size;

    chunk.msgs.push_back(message);
  }
  return chunk;
}

MCAPClient::MCAPClient(QObject* parent) : QObject(parent)
{
  connect(&_socket, &QWebSocket::connected, this, &MCAPClient::connected);
  connect(&_socket, &QWebSocket::disconnected, this, &MCAPClient::disconnected);
  connect(&_socket, qOverload<QAbstractSocket::SocketError>(&QWebSocket::error), this,
          [this](QAbstractSocket::SocketError error) {
            qDebug() << "MCAPClient error: " << error;
            auto errorString = _socket.errorString();
            emit errorOccured(errorString);
          });

  connect(&_socket, &QWebSocket::textMessageReceived, this,
          &MCAPClient::onTextMessageReceived);
  connect(&_socket, &QWebSocket::binaryMessageReceived, this,
          &MCAPClient::onBinaryMessageReceived);
}

MCAPClient::~MCAPClient()
{
  _socket.close();
}

void MCAPClient::connectToServer(QString host, int port)
{
  if (isConnected())
  {
    disconnectFromServer();
  }
  QUrl url;
  url.setScheme("ws");
  url.setHost(host);
  url.setPort(port);
  _socket.open(url);
}

void MCAPClient::disconnectFromServer()
{
  _socket.close();
}

bool MCAPClient::isConnected() const
{
  return _socket.isValid();
}

void MCAPClient::requestFileList()
{
  auto json = nlohmann::json();
  json["header"] = nlohmann::json{ { "type", mcap_protocol::REQUEST_FILE_LIST } };
  sendRequest(json);
}

void MCAPClient::requestFileStatistics(QString filename)
{
  auto json = nlohmann::json();
  json["header"] = nlohmann::json{ { "type", mcap_protocol::REQUEST_STATISTICS } };
  json["body"] = nlohmann::json{ { "filename", filename.toStdString() } };
  sendRequest(json);
}

void MCAPClient::downloadData(const mcap_api::DataRequest& request)
{
  auto json = nlohmann::json();
  json["header"] = nlohmann::json{ { "type", mcap_protocol::DOWNLOAD_DATA } };
  json["body"] = mcap_protocol::toJson(request);
  sendRequest(json);
}

void MCAPClient::cancelDownload()
{
  auto json = nlohmann::json();
  json["header"] = nlohmann::json{ { "type", mcap_protocol::CANCEL_DOWNLOAD } };
  sendRequest(json);
}

void MCAPClient::sendRequest(const nlohmann::json& json)
{
  _socket.sendTextMessage(QString::fromStdString(json.dump()));
}

void MCAPClient::onFileListReplied(const nlohmann::json& body)
{
  if (body.is_structured())
  {
    auto files = body.at("files");
    if (files.is_array())
    {
      std::vector<mcap_api::FileInfo> files_info;
      for (const auto& element : files)
      {
        mcap_api::FileInfo info;
        mcap_protocol::fromJson(info, element);
        files_info.push_back(std::move(info));
      }
      emit fileListReceived(files_info);
    }
  }
}

void MCAPClient::onStatisticsReplied(const nlohmann::json& body)
{
  if (body.is_structured())
  {
    mcap_api::Statistics statistics;
    mcap_protocol::fromJson(statistics, body);
    emit statisticsReceived(statistics);
  }
}

void MCAPClient::onDownloadDataReplied(const nlohmann::json& body)
{
  if (body.is_structured())
  {
    int size = -1;
    body.at("chunksSize").get_to(size);
    emit transferStarted(size);
  }
}

void MCAPClient::onTextMessageReceived(const QString &message)
{
  auto json = nlohmann::json::parse(message.toStdString());
  auto header = json.at("header");
  std::string type;
  if (header.is_structured())
  {
    auto typeValue = header.at("type");
    if (typeValue.is_string())
    {
      type = typeValue.get<std::string>();
    }
  }

  nlohmann::json body;
  auto bodyIt = json.find("body");
  if (bodyIt != json.end())
  {
    body = *bodyIt;
  }

  try
  {
    if (type == mcap_protocol::REQUEST_FILE_LIST)
    {
      onFileListReplied(body);
    }
    else if (type == mcap_protocol::REQUEST_STATISTICS)
    {
      onStatisticsReplied(body);
    }
    else if (type == mcap_protocol::DOWNLOAD_DATA)
    {
      onDownloadDataReplied(body);
    }
    else if (type == mcap_protocol::CANCEL_DOWNLOAD)
    {
      emit cancelReceived();
    }
    else if (type == mcap_protocol::TRANSFER_STARTED)
    {
      emit transferStarted(body["chunkCount"].get<int>());
    }
    else if (type == mcap_protocol::TRANSFER_COMPLETED)
    {
      emit transferCompleted( );
    }
    else if (type == mcap_protocol::ERROR_MSG)
    {
      QMessageBox::warning(nullptr, "Error from server",
                           QString::fromStdString(body.get<std::string>()));
    }
    else {
      qDebug() << "received unrecognized message: " << QString::fromStdString(type);
    }
  }
  catch(std::exception& err)
  {
    std::cout << body.dump(2) << std::endl;
    QMessageBox::warning(nullptr, "Exception in Client", QString(err.what()));
  }
}


void MCAPClient::onBinaryMessageReceived(const QByteArray& message)
{
  if( strcmp("dataChunk", message.data()) != 0 )
  {
    qDebug() << "Unexpected header in binary message";
    return;
  }

  const auto chunk = deserializeChunk(message);
  emit chunkReceived(chunk);
}

}  // namespace mcap_client

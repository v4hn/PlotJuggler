#pragma once

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QtWebSockets/QtWebSockets>

#include "mcap_api/mcap_protocol.h"

class QWebSocketServer;
class QWebSocket;

namespace mcap_client
{

class MCAPClient : public QObject
{
  Q_OBJECT
public:
  explicit MCAPClient(QObject* parent = nullptr);
  ~MCAPClient() override;

  /**
   * @brief connectToServer connect to localhost server with specified port
   *
   * connected will be emitted upon connecting
   * @param port server listening port
   */
  void connectToServer(QString host = "localhost", int port = 1667);

  /*!
   * \brief disconnect disconnect from current connected server
   *
   * disconnected will be emitted upon disconnecting
   */
  void disconnectFromServer();

  bool isConnected() const;

signals:
  void connected();

  void disconnected();

  void errorOccured(QString errorString);

  void fileListReceived(const std::vector<mcap_api::FileInfo>& info);

  void statisticsReceived(const mcap_api::Statistics& statistics);

  void transferStarted(int chunkCount);

  void chunkReceived(const mcap_api::ChunkView &chunk);

  void transferCompleted();

  /**
   * @brief cancelReceived emitted when server received the cancel request and
   * the request is successfully cancelled
   */
  void cancelReceived();

public slots:

  void requestFileList();

  void requestFileStatistics(QString filename);

  void downloadData(const mcap_api::DataRequest& request);

  void cancelDownload();

private slots:

  void onTextMessageReceived(const QString& message);

  void onBinaryMessageReceived(const QByteArray &message);

  void onFileListReplied(const nlohmann::json& body);

  void onStatisticsReplied(const nlohmann::json& body);

  void onDownloadDataReplied(const nlohmann::json& body);

private:
  QWebSocket _socket;

  std::map<std::string, mcap_api::Statistics> _statistics_by_file;

  void sendRequest(const nlohmann::json& json);
};

}  // namespace mcap_client

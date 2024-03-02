#include "toolbox_remote.h"

#include <QMessageBox>
#include <array>
#include <math.h>

#define MCAP_IMPLEMENTATION
#include "mcap/writer.hpp"

#include "ui_dialog_file_selection.h"

ToolboxRemote::ToolboxRemote() : _widget(new RemoteLoad)
{
  connect(_widget, &RemoteLoad::openFile, this, [this](QString host, int port) {
    _client.connectToServer(host, port);
  });

  connect(&_client, &mcap_client::MCAPClient::connected, this, [this]() {
    _client.requestFileList();
  });

  connect(&_client, &mcap_client::MCAPClient::disconnected, _widget, [this]() {
    _widget->clear();

    if(_progress_dialog)
    {
      _progress_dialog->close();
      _progress_dialog = nullptr;
    }
  });

  connect(&_client, &mcap_client::MCAPClient::statisticsReceived,
          this, &ToolboxRemote::onFileStatisticsReceived);

  connect(&_client, &mcap_client::MCAPClient::errorOccured, _widget, [this](QString error) {
    QMessageBox::warning(nullptr, "Error connecting to server", error);
    emit _client.disconnected();
  });

  connect(&_client, &mcap_client::MCAPClient::fileListReceived,
          this, &ToolboxRemote::onShowFilesToSelect);

  connect(_widget, &RemoteLoad::rejected, this, [this]() {
    if(_mcap_writer)
    {
      _mcap_writer->close();
      _mcap_writer.reset();
    }
    emit closed();
  });

  connect(_widget, &RemoteLoad::loadTopics, this, &ToolboxRemote::onLoadTopics);

  connect(&_client, &mcap_client::MCAPClient::transferStarted,
          [this](int count) {
            qDebug() << "transferStarted";
            _progress_dialog->setWindowTitle("Transfering and parsing data");
            _progress_dialog->setLabelText("Almost there...");
            _progress_dialog->setRange(0, count - 1);
            _progress_dialog->setValue(0);
            _progress_dialog->setAutoClose(true);
          });

  connect(&_client, &mcap_client::MCAPClient::transferCompleted,
          [this]() {
            emit importData(_plot_data, true);
            _plot_data.clear();
            qDebug() << "transferCompleted";
            if(_progress_dialog)
            {
              _progress_dialog->close();
              _progress_dialog = nullptr;
            }
            if(_mcap_writer)
            {
              _mcap_writer->close();
              _mcap_writer.reset();
            }
            emit closed();
          });

  connect(&_client, &mcap_client::MCAPClient::chunkReceived,
          this, &ToolboxRemote::onChunkReceived);
}

ToolboxRemote::~ToolboxRemote()
{
  delete _widget;
}

void ToolboxRemote::init(PJ::PlotDataMapRef& /*src_data*/, PJ::TransformsMap& /*transform_map*/)
{
}

std::pair<QWidget*, PJ::ToolboxPlugin::WidgetType> ToolboxRemote::providedWidget() const
{
  return { _widget, PJ::ToolboxPlugin::FIXED };
}

bool ToolboxRemote::onShowWidget()
{
  return true;
}

void ToolboxRemote::onShowFilesToSelect(const std::vector<mcap_api::FileInfo>& infos)
{
  QDialog* dialog = new QDialog(_widget);
  auto ui = new Ui::DialogFileSelection();
  ui->setupUi(dialog);

  for(auto const& info: infos)
  {
    ui->tableWidget->insertRow(0);

    auto item_name = new QTableWidgetItem(QString::fromStdString(info.filename));
    ui->tableWidget->setItem(0, 0, item_name);

    auto item_size = new QTableWidgetItem(QString("%0 MB").arg(info.size_MB, 0, 'f', 1));
    ui->tableWidget->setItem(0, 1, item_size);

    auto item_date = new QTableWidgetItem(QString::fromStdString(info.dateTime));
    ui->tableWidget->setItem(0, 2, item_date);
  }

  QHeaderView* header = ui->tableWidget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::Fixed);
  header->setSectionResizeMode(2, QHeaderView::Fixed);

  ui->tableWidget->resizeColumnToContents(1);
  ui->tableWidget->resizeColumnToContents(2);
  ui->tableWidget->sortByColumn(0);

  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  connect(ui->tableWidget, &QTableWidget::itemSelectionChanged,
          [ui]() {
            bool selected = !ui->tableWidget->selectedItems().empty();
            ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(selected);
          });

  connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, dialog,
          [dialog](int, int) { dialog->accept(); });

  _current_filename.clear();

  dialog->setWindowTitle("Select the file");

  if (dialog->exec() == QDialog::Accepted)
  {
    QItemSelectionModel *select = ui->tableWidget->selectionModel();
    if(select->hasSelection())
    {
      int row = select->selectedRows().front().row();
      _current_filename = ui->tableWidget->item(row, 0)->text();
      _client.requestFileStatistics(_current_filename);
      _widget->setFileName(_current_filename);
    }
  }
  dialog->setAttribute(Qt::WA_DeleteOnClose);
}

void ToolboxRemote::onFileStatisticsReceived(const mcap_api::Statistics &statistics)
{
  _statistics = statistics;

  _parsers_by_channel.clear();

  std::set<QString> missing_encoding_notified;

  for(const auto& channel: _statistics.channels)
  {
    const QString encoding = QString::fromStdString(channel.schema_encoding);
    auto it = parserFactories()->find( encoding );

    if(it != parserFactories()->end() )
    {
      auto& parser_factory = it->second;

      std::string schema(reinterpret_cast<const char*>(channel.schema_data.data()),
                         channel.schema_data.size());

      auto parser = parser_factory->createParser(channel.topic,
                                                 channel.schema_name,
                                                 schema,
                                                 _plot_data);
      _parsers_by_channel.insert( {channel.id, parser} );
    }
    else {
      if( missing_encoding_notified.count(encoding) == 0)
      {
        missing_encoding_notified.insert(encoding);
        QMessageBox::warning(_widget, "parser Factory",
                             QString("Can't find a parser for messages with encoding %0").arg(encoding));
      }
    }
  }

  _widget->enableTopics(true);
  _widget->setChannels(statistics.channels);
  _widget->setMaxTimeRange( 1e-9 * double(statistics.end_time - statistics.start_time) );
}

void ToolboxRemote::onLoadTopics(QStringList topics)
{
  mcap_api::DataRequest request;

  for(auto& [id, parser]: _parsers_by_channel)
  {
    parser->setLargeArraysPolicy( _widget->clampLargeArrays(), _widget->maxArraySize() );
  }

  _plot_data.clear();

  for(auto const& topic: topics)
  {
    request.topics.push_back( topic.toStdString() );
  }
  request.filename = _current_filename.toStdString();
  const auto range = _widget->getTimeRange();
  request.start_time = _statistics.start_time + uint64_t(1e9 * range.first);
  request.end_time = _statistics.start_time + uint64_t(1e9 * range.second);

  _client.downloadData(request);

  _progress_dialog = new QProgressDialog(_widget);
  _progress_dialog->setLabelText("This may take a while, based on the size of the file");
  _progress_dialog->setWindowTitle("Extracting data from file");
  _progress_dialog->setRange(0, 0);
  _progress_dialog->setValue(0);
  _progress_dialog->show();
  _progress_dialog->setAttribute(Qt::WA_DeleteOnClose);

  connect(_progress_dialog, &QProgressDialog::canceled,
          &_client, &mcap_client::MCAPClient::cancelDownload);


  auto filepath = _widget->getSaveFile();
  if(!filepath.isEmpty())
  {
    saveData(filepath, topics);
  }
}

void ToolboxRemote::onChunkReceived(const mcap_api::ChunkView &chunk)
{
  _progress_dialog->setValue(chunk.index);

  for(const auto& msg_view: chunk.msgs)
  {
    double timestamp_sec = double(msg_view.timestamp) * 1e-9;

    if(_mcap_writer)
    {
      mcap::Message msg;
      const auto channel_id = _channeld_id_remapping.at(msg_view.channel_id);
      msg.channelId = channel_id;
      msg.logTime = msg_view.timestamp;
      msg.publishTime = msg_view.timestamp;
      msg.data = msg_view.data;
      msg.dataSize = msg_view.data_size;
      auto res = _mcap_writer->write(msg);
    }

    auto parser_it = _parsers_by_channel.find(msg_view.channel_id);
    if( parser_it == _parsers_by_channel.end() )
    {
      continue;
    }

    auto parser = parser_it->second;
    MessageRef msg(msg_view.data, msg_view.data_size);
    parser->parseMessage(msg, timestamp_sec);
  }
}

bool ToolboxRemote::saveData(QString filepath, QStringList topics)
{
  _mcap_writer = std::make_shared<mcap::McapWriter>();

  auto options = mcap::McapWriterOptions("MCAP PlotJuggler Cache");
  const auto res = _mcap_writer->open(filepath.toStdString(), options);
  if (!res.ok())
  {
    QMessageBox::warning(_widget, "MCAP cache saving",
                         QString("Failed to open the file: \n%0").arg(filepath));
    return false;
  }

  for(auto const& info: _statistics.channels)
  {
    if(!topics.contains(QString::fromStdString(info.topic)))
    {
      continue;
    }

    mcap::Schema schema(info.schema_name,
                        info.schema_encoding,
                        info.schema_data);
    _mcap_writer->addSchema(schema);
    mcap::Channel channel(info.topic, info.message_encoding, schema.id);
    _mcap_writer->addChannel(channel);
    _channeld_id_remapping.insert({ info.id, channel.id});
  }

  return true;
}

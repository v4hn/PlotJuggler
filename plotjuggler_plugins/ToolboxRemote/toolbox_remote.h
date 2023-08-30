#pragma once

#include <QtPlugin>
#include <QProgressDialog>
#include <thread>
#include "PlotJuggler/toolbox_base.h"
#include "PlotJuggler/plotwidget_base.h"
#include "PlotJuggler/messageparser_base.h"
#include "widget_remote_load.hpp"
#include "ui_widget_remote_load.h"
#include "mcap_client/mcap_client.h"

#include <QDialog>

namespace mcap
{
class McapWriter;
}

class ToolboxRemote : public PJ::ToolboxPlugin
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.Toolbox")
  Q_INTERFACES(PJ::ToolboxPlugin)

public:
  ToolboxRemote();

  ~ToolboxRemote() override;

  const char* name() const override
  {
    return "Load Data from remote server";
  }

  void init(PJ::PlotDataMapRef& src_data, PJ::TransformsMap& transform_map) override;

  std::pair<QWidget*, WidgetType> providedWidget() const override;

public slots:

  bool onShowWidget() override;

private slots:

  void onShowFilesToSelect(const std::vector<mcap_api::FileInfo> &info);

  void onFileStatisticsReceived(const mcap_api::Statistics &statistics);

  void onLoadTopics(QStringList topics);

  void onChunkReceived(const mcap_api::ChunkView &chunk);

private:
  RemoteLoad* _widget;

  mcap_client::MCAPClient _client;
  mcap_api::Statistics _statistics;

  std::unordered_map<int, MessageParserPtr> _parsers_by_channel; // channel_id

  std::unordered_set<std::string> _series_names;

  PJ::PlotDataMapRef _plot_data;

  QProgressDialog* _progress_dialog = nullptr;

  QString _current_filename;

  std::shared_ptr<mcap::McapWriter> _mcap_writer;
  // from old ID to new one
  std::unordered_map<int, int> _channeld_id_remapping;

  bool saveData(QString filepath, QStringList topics);
};

#pragma once

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"

#include <zcm/zcm-cpp.hpp>

#include <zcm/tools/TypeDb.hpp>
#include <zcm/tools/Introspection.hpp>

#include "ui_datastream_zcm.h"

class DataStreamZcm : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamZcm();

  virtual ~DataStreamZcm();

  virtual const char* name() const override;

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override;

  virtual bool xmlSaveState(QDomDocument& doc,
                            QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;

private:
  std::unique_ptr<zcm::TypeDb> _types;

  std::unique_ptr<zcm::ZCM> _zcm;

  zcm::Subscription* _subs = nullptr;

  zcm::Introspection::ProcessFn _processData;

  std::vector<std::pair<std::string, double>> _numerics;
  std::vector<std::pair<std::string, std::string>> _strings;

  void handler(const zcm::ReceiveBuffer* rbuf, const std::string& channel);

  bool _running;
  QString _types_folder;
  QString _subscribe_string;

  QDialog* _dialog;
  Ui::DialogZcm* _ui;
};

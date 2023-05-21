#pragma once

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"

#include <zcm/zcm-cpp.hpp>

#include <zcm/tools/TypeDb.hpp>

class DataStreamZcm : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  DataStreamZcm();

  virtual ~DataStreamZcm();

  virtual bool start(QStringList*) override;

  virtual void shutdown() override;

  virtual bool isRunning() const override;

  virtual const char* name() const override
  {
    return "Zcm Streamer";
  }

  virtual bool isDebugPlugin() override
  {
    return false;
  }

  virtual bool xmlSaveState(QDomDocument& doc,
                            QDomElement& parent_element) const override;

  virtual bool xmlLoadState(const QDomElement& parent_element) override;

  std::pair<QAction*, int> notificationAction() override
  {
    return { _dummy_notification, _notifications_count };
  }

private:
  std::unique_ptr<zcm::TypeDb> _types;

  std::unique_ptr<zcm::ZCM> _zcm;

  zcm::Subscription* _subs;

  void handler(const zcm::ReceiveBuffer* rbuf, const std::string& channel);

  bool _running;

  QAction* _dummy_notification;

  int _notifications_count;
};

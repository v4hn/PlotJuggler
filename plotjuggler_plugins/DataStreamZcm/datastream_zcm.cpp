#include "datastream_zcm.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

#include <iomanip>
#include <iostream>
#include <mutex>

#include <zcm/tools/Introspection.hpp>

using namespace std;
using namespace PJ;

DataStreamZcm::DataStreamZcm() : _running(false), _subs(nullptr)
{
  _dummy_notification = new QAction(this);

  connect(_dummy_notification, &QAction::triggered, this, [this]() {
    QMessageBox::warning(nullptr, "Dummy Notifications",
                         QString("%1 notifications").arg(_notifications_count),
                         QMessageBox::Ok);

    if (_notifications_count > 0)
    {
      _notifications_count = 0;
      emit notificationsChanged(_notifications_count);
    }
  });

  _notifications_count = 0;

  _types.reset(new zcm::TypeDb(getenv("ZCMTYPES_PATH")));
  assert(_types->good() && "Failed to load zcmtypes");

  _zcm.reset(new zcm::ZCM(""));
  if (_zcm->good()) _subs = _zcm->subscribe(".*", &DataStreamZcm::handler, this);
}

bool DataStreamZcm::start(QStringList*)
{
  if (_running) return false;
  if (_zcm->good()) {
    _zcm->start();
    _running = true;
  }
  return true;
}

void DataStreamZcm::shutdown()
{
  if (!_running) return;
  _zcm->stop();
  _running = false;
}

bool DataStreamZcm::isRunning() const
{
  return _running;
}

DataStreamZcm::~DataStreamZcm()
{
  shutdown();
}

bool DataStreamZcm::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataStreamZcm::xmlLoadState(const QDomElement& parent_element)
{
  return true;
}

void DataStreamZcm::handler(const zcm::ReceiveBuffer* rbuf, const string& channel)
{
  vector<pair<string, double>> numerics;
  vector<pair<string, string>> strings;

  auto processScalar = [&numerics, &strings](const string& name,
                                             zcm_field_type_t type,
                                             const void* data){
    switch (type) {
      case ZCM_FIELD_INT8_T: numerics.emplace_back(name, *((int8_t*)data)); break;
      case ZCM_FIELD_INT16_T: numerics.emplace_back(name, *((int16_t*)data)); break;
      case ZCM_FIELD_INT32_T: numerics.emplace_back(name, *((int32_t*)data)); break;
      case ZCM_FIELD_INT64_T: numerics.emplace_back(name, *((int64_t*)data)); break;
      case ZCM_FIELD_BYTE: numerics.emplace_back(name, *((uint8_t*)data)); break;
      case ZCM_FIELD_FLOAT: numerics.emplace_back(name, *((float*)data)); break;
      case ZCM_FIELD_DOUBLE: numerics.emplace_back(name, *((double*)data)); break;
      case ZCM_FIELD_BOOLEAN: numerics.emplace_back(name, *((bool*)data)); break;
      case ZCM_FIELD_STRING: strings.emplace_back(name, string((const char*)data)); break;
      case ZCM_FIELD_USER_TYPE: assert(false && "Should not be possble");
    }
  };

  zcm::Introspection::processEncodedType(channel,
                                         rbuf->data, rbuf->data_size,
                                         "/",
                                         *_types.get(), processScalar);

  /*
  for (auto& n : numerics) cout << setprecision(10) << n.first << "," << n.second << endl;
  for (auto& s : strings) cout << s.first << "," << s.second << endl;
  */

  {
    std::lock_guard<std::mutex> lock(mutex());

    for (auto& n : numerics) {
        if (dataMap().numeric.find(n.first) == dataMap().numeric.end())
            dataMap().addNumeric(n.first);
        auto& series = dataMap().numeric.find(n.first)->second;
        series.pushBack({ (double)rbuf->recv_utime / 1e6, n.second });
    }
    for (auto& s : strings) {
        if (dataMap().strings.find(s.first) == dataMap().strings.end())
            dataMap().addStringSeries(s.first);
        auto& series = dataMap().strings.find(s.first)->second;
        series.pushBack({ (double)rbuf->recv_utime / 1e6, s.second });
    }
  }

  emit dataReceived();
}

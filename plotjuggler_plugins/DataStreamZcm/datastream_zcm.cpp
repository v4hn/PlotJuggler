#include "datastream_zcm.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

#include <iomanip>
#include <iostream>
#include <mutex>

using namespace std;
using namespace PJ;

DataStreamZcm::DataStreamZcm() : _running(false), _subs(nullptr)
{
  _processData = [&](const string& name, zcm_field_type_t type, const void* data){
    switch (type) {
      case ZCM_FIELD_INT8_T: _numerics.emplace_back(name, *((int8_t*)data)); break;
      case ZCM_FIELD_INT16_T: _numerics.emplace_back(name, *((int16_t*)data)); break;
      case ZCM_FIELD_INT32_T: _numerics.emplace_back(name, *((int32_t*)data)); break;
      case ZCM_FIELD_INT64_T: _numerics.emplace_back(name, *((int64_t*)data)); break;
      case ZCM_FIELD_BYTE: _numerics.emplace_back(name, *((uint8_t*)data)); break;
      case ZCM_FIELD_FLOAT: _numerics.emplace_back(name, *((float*)data)); break;
      case ZCM_FIELD_DOUBLE: _numerics.emplace_back(name, *((double*)data)); break;
      case ZCM_FIELD_BOOLEAN: _numerics.emplace_back(name, *((bool*)data)); break;
      case ZCM_FIELD_STRING: _strings.emplace_back(name, string((const char*)data)); break;
      case ZCM_FIELD_USER_TYPE: assert(false && "Should not be possble");
    }
  };

  _types.reset(new zcm::TypeDb(getenv("ZCMTYPES_PATH")));
  assert(_types->good() && "Failed to load zcmtypes");

  _zcm.reset(new zcm::ZCM(""));
  if (_zcm->good()) _subs = _zcm->subscribe(".*", &DataStreamZcm::handler, this);
}

DataStreamZcm::~DataStreamZcm()
{
  shutdown();
}

const char* DataStreamZcm::name() const
{
  return "Zcm Streamer";
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
  zcm::Introspection::processEncodedType(channel,
                                         rbuf->data, rbuf->data_size,
                                         "/",
                                         *_types.get(), _processData);

  /*
  for (auto& n : _numerics) cout << setprecision(10) << n.first << "," << n.second << endl;
  for (auto& s : _strings) cout << s.first << "," << s.second << endl;
  */

  {
    std::lock_guard<std::mutex> lock(mutex());

    for (auto& n : _numerics) {
        auto itr = dataMap().numeric.find(n.first);
        if (itr == dataMap().numeric.end()) itr = dataMap().addNumeric(n.first);
        itr->second.pushBack({ (double)rbuf->recv_utime / 1e6, n.second });
    }
    for (auto& s : _strings) {
        auto itr = dataMap().strings.find(s.first);
        if (itr == dataMap().strings.end()) itr = dataMap().addStringSeries(s.first);
        itr->second.pushBack({ (double)rbuf->recv_utime / 1e6, s.second });
    }
  }

  emit dataReceived();

  _numerics.clear();
  _strings.clear();
}

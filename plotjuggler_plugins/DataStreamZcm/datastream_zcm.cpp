#include "datastream_zcm.h"

#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>

#include <iomanip>
#include <iostream>
#include <mutex>

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>

using namespace std;
using namespace PJ;

template <typename T>
double toDouble(const void* data) {
  return static_cast<double>(*reinterpret_cast<const T*>(data));
}

DataStreamZcm::DataStreamZcm(): _subs(nullptr), _running(false)
{
  _processData = [&](const string& name, zcm_field_type_t type, const void* data){
    switch (type) {
    case ZCM_FIELD_INT8_T: _numerics.emplace_back(name, toDouble<int8_t>(data)); break;
      case ZCM_FIELD_INT16_T: _numerics.emplace_back(name, toDouble<int16_t>(data)); break;
      case ZCM_FIELD_INT32_T: _numerics.emplace_back(name, toDouble<int32_t>(data)); break;
      case ZCM_FIELD_INT64_T: _numerics.emplace_back(name, toDouble<int64_t>(data)); break;
      case ZCM_FIELD_BYTE: _numerics.emplace_back(name, toDouble<uint8_t>(data)); break;
      case ZCM_FIELD_FLOAT: _numerics.emplace_back(name, toDouble<float>(data)); break;
      case ZCM_FIELD_DOUBLE: _numerics.emplace_back(name, toDouble<double>(data)); break;
      case ZCM_FIELD_BOOLEAN: _numerics.emplace_back(name, toDouble<bool>(data)); break;
      case ZCM_FIELD_STRING: _strings.emplace_back(name, string((const char*)data)); break;
      case ZCM_FIELD_USER_TYPE: assert(false && "Should not be possble");
    }
  };

  _dialog = new QDialog;
  _ui = new Ui::DialogZcm;
  _ui->setupUi(_dialog);

  // When the "Select" button is pushed, open a file dialog to select
  // a different folder
  connect(_ui->buttonSelectFolder, &QPushButton::clicked, this,
          [this](){
              QString filename = QFileDialog::getOpenFileName(
                  nullptr, tr("Select ZCM Type File"),
                  {}, "*.so");
              // if valid, update lineEditFolder
              if(!filename.isEmpty()) {
                  _ui->lineEditFolder->setText(filename);
              }
          });
  // When the "Default" button is pushed, load from getenv("ZCMTYPES_PATH")
  connect(_ui->buttonDefaultFolder, &QPushButton::clicked, this,
          [this](){
            QString folder = getenv("ZCMTYPES_PATH");
            if(folder.isEmpty()){
              QMessageBox::warning(nullptr, "Error", "Environment variable ZCMTYPES_PATH not set");
            }
            else {
              _ui->lineEditFolder->setText(getenv("ZCMTYPES_PATH"));
            }
          });
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
  if (_running) {
    return false;
  }

  // Initialize zmc here, only once if everything goes well
  if(!_zcm) {
    _zcm.reset(new zcm::ZCM("ipc"));

    if (!_zcm->good()) {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::ZCM()");
      _zcm.reset();
      _subs = nullptr;
      // QUESTION: to we need to call first "delete _subs" ?
      // Who have the ownership of that pointer?
      return false;
    }
  }

  // We create a Dialog to request the folder to populate zcm::TypeDb
  // and the string to pass to the subscriber.

  // Initialize the lineEdits in the ui with the previous value;
  QSettings settings;
  auto const prev_folder = settings.value("DataStreamZcm::folder",
                                          getenv("ZCMTYPES_PATH")).toString();
  _ui->lineEditFolder->setText(prev_folder);
  auto const subscribe_text = settings.value("DataStreamZcm::subscribe", ".*").toString();
  _ui->lineEditSubscribe->setText(subscribe_text);

  // start the dialog and check that OK was pressed
  _dialog->restoreGeometry(settings.value("DataStreamZcm::geometry").toByteArray());
  int res = _dialog->exec();

  settings.setValue("DataStreamZcm::geometry", _dialog->saveGeometry());
  if (res == QDialog::Rejected) {
    return false;
  }

  // save the current configuration for the next execution
  settings.setValue("DataStreamZcm::folder", _ui->lineEditFolder->text());
  settings.setValue("DataStreamZcm::subscribe", _ui->lineEditSubscribe->text());

  // reset the types if it is the first time or folder changed
  if(_types_folder != _ui->lineEditFolder->text() || !_types)
  {
    _types_folder = _ui->lineEditFolder->text();
    _types.reset(new zcm::TypeDb(_types_folder.toStdString()));
    if(!_types->good()) {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::TypeDb()");
      _types.reset();
      return false;
    }
  }

  if(_subscribe_string != _ui->lineEditSubscribe->text() || !_subs)
  {
    _subscribe_string = _ui->lineEditSubscribe->text();
    _subs =_zcm->subscribe(_subscribe_string.toStdString(), &DataStreamZcm::handler, this);
    if (!_zcm->good()) {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::TypeDb()");
      // QUESTION: is there any cleanup that need to be done here?
      return false;
    }
  }

  _zcm->start();
  _running = true;
  return true;
}

void DataStreamZcm::shutdown()
{
  if (!_running) {
    return;
  }
  _zcm->stop();
  _running = false;
}

bool DataStreamZcm::isRunning() const
{
  return _running;
}

bool DataStreamZcm::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("config");
  elem.setAttribute("folder", _types_folder);
  elem.setAttribute("subscribe", _subscribe_string);
  parent_element.appendChild(elem);
  return true;
}

bool DataStreamZcm::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("config");
  if (!elem.isNull())
  {
    _types_folder = elem.attribute("folder");
    _subscribe_string = elem.attribute("subscribe");
    QSettings settings;
    settings.setValue("DataStreamZcm::folder", _types_folder);
    settings.setValue("DataStreamZcm::subscribe", _subscribe_string);
  }
  return true;
}

void DataStreamZcm::handler(const zcm::ReceiveBuffer* rbuf, const string& channel)
{
  zcm::Introspection::processEncodedType(channel,
                                         rbuf->data, rbuf->data_size,
                                         "/",
                                         *_types.get(), _processData);
  {
    std::lock_guard<std::mutex> lock(mutex());

    for (auto& n : _numerics) {
        auto itr = dataMap().numeric.find(n.first);
        if (itr == dataMap().numeric.end()) {
          itr = dataMap().addNumeric(n.first);
        }
        itr->second.pushBack({ double(rbuf->recv_utime) / 1e6, n.second });
    }
    for (auto& s : _strings) {
        auto itr = dataMap().strings.find(s.first);
        if (itr == dataMap().strings.end()) {
          itr = dataMap().addStringSeries(s.first);
        }
        itr->second.pushBack({ double(rbuf->recv_utime) / 1e6, s.second });
    }
  }

  emit dataReceived();

  _numerics.clear();
  _strings.clear();
}

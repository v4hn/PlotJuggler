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

DataStreamZcm::DataStreamZcm(): _subs(nullptr), _running(false)
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
    _zcm.reset(new zcm::ZCM(""));
    if (!_zcm->good()) {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::ZCM()");
      _zcm.reset();
      _subs = nullptr;
      // QUESTION: to we need to call first "delete _subs" ?
      // Who have the ownership ofthat pointer?
      return false;
    }
  }

  // We create a Dialog to request the folder to populate zcm::TypeDb
  // and the string to pass to the subscriber.
  auto dialog = new QDialog;
  auto ui = new Ui::DialogZmc;
  ui->setupUi(dialog);

  // Initialize the lineEdits in the ui with the previous value;
  QSettings settings;
  auto const prev_folder = settings.value("DataStreamZcm::folder", getenv("ZCMTYPES_PATH")).toString();
  ui->lineEditFolder->setText(prev_folder);
  auto const subscribe_text = settings.value("DataStreamZcm::subscribe", ".*").toString();
  ui->lineEditSubscribe->setText(subscribe_text);

  // When the "Change..." button is pushed, open a file dialog to select
  // a different folder
  connect(ui->pushButtonChangeFolder, &QPushButton::clicked, dialog,
          [ui](){
            QString dir = QFileDialog::getExistingDirectory(
                nullptr, tr("Select Directory"),
                ui->lineEditFolder->text(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
            // if valid, update lineEditFolder
            if(!dir.isEmpty()) {
              ui->lineEditFolder->setText(dir);
            }
          });
  // start the dialog and check that OK was pressed
  int res = dialog->exec();
  if (res == QDialog::Rejected) {
    return false;
  }
  // save the current configuration for hte next time
  settings.setValue("DataStreamZcm::folder", ui->lineEditFolder->text());
  settings.setValue("DataStreamZcm::subscribe", ui->lineEditSubscribe->text());
  dialog->deleteLater();

  // reset the types if it is the first time or folder changed
  if(_types_folder != ui->lineEditFolder->text() || !_types)
  {
    _types_folder = ui->lineEditFolder->text();
    _types.reset(new zcm::TypeDb(_types_folder.toStdString()));
    if(!_types->good()) {
      QMessageBox::warning(nullptr, "Error", "Failed to create zcm::TypeDb()");
      _types.reset();
      return false;
    }
  }

  if(_subscribe_string != ui->lineEditSubscribe->text() || !_subs)
  {
    _subscribe_string = ui->lineEditSubscribe->text();
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

  /*
  for (auto& n : _numerics) cout << setprecision(10) << n.first << "," << n.second << endl;
  for (auto& s : _strings) cout << s.first << "," << s.second << endl;
  */

  {
    std::lock_guard<std::mutex> lock(mutex());

    for (auto& n : _numerics) {
        auto itr = dataMap().numeric.find(n.first);
        if (itr == dataMap().numeric.end()) {
          itr = dataMap().addNumeric(n.first);
        }
        itr->second.pushBack({ (double)rbuf->recv_utime / 1e6, n.second });
    }
    for (auto& s : _strings) {
        auto itr = dataMap().strings.find(s.first);
        if (itr == dataMap().strings.end()) {
          itr = dataMap().addStringSeries(s.first);
        }
        itr->second.pushBack({ (double)rbuf->recv_utime / 1e6, s.second });
    }
  }

  emit dataReceived();

  _numerics.clear();
  _strings.clear();
}

#include "dataload_zcm.h"

#include <QDebug>
#include <QFile>
#include <QInputDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QTextStream>
#include <QWidget>
#include <QFileDialog>

#include <iostream>

#include <zcm/zcm-cpp.hpp>
#include <zcm/tools/Introspection.hpp>

using namespace std;

DataLoadZcm::DataLoadZcm()
{
  _dialog = new QDialog();
  _ui = new Ui::DialogZcm();
  _ui->setupUi(_dialog);

  _config_widget = new ConfigZCM("DataLoadZcm", _dialog);
  _ui->mainLayout->insertWidget(0, _config_widget, 1);

  _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

  _ui->listWidgetChannels->setSelectionMode(QAbstractItemView::ExtendedSelection);

  connect(_ui->listWidgetChannels, &QListWidget::itemSelectionChanged, this, [this]() {
      auto selected = _ui->listWidgetChannels->selectionModel()->selectedIndexes();
    bool box_enabled = selected.size() > 0;
    _ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(box_enabled);
  });
}

DataLoadZcm::~DataLoadZcm()
{
  delete _dialog;
}

const char* DataLoadZcm::name() const
{
  return "DataLoad Zcm";
}

const vector<const char*>& DataLoadZcm::compatibleFileExtensions() const
{
  static vector<const char*> extensions = { "zcmlog" };
  return extensions;
}

static int processInputLog(const string& logpath,
                           function<void(const zcm::LogEvent* evt)> processEvent)
{
    zcm::LogFile inlog(logpath, "r");
    if (!inlog.good()) {
        cerr << "Unable to open input zcm log: " << logpath << endl;
        return 1;
    }

    auto processLog = [&inlog](function<void(const zcm::LogEvent* evt)> processEvent) {
        const zcm::LogEvent* evt;
        off64_t offset;
        static int lastPrintPercent = 0;

        fseeko(inlog.getFilePtr(), 0, SEEK_END);
        off64_t logSize = ftello(inlog.getFilePtr());
        fseeko(inlog.getFilePtr(), 0, SEEK_SET);

        QProgressDialog progress_dialog;
        progress_dialog.setLabelText("Loading... please wait");
        progress_dialog.setWindowModality(Qt::ApplicationModal);
        progress_dialog.setRange(0, 100);
        progress_dialog.setAutoClose(true);
        progress_dialog.setAutoReset(true);
        progress_dialog.show();

        bool interrupted = false;

        while (1) {
            offset = ftello(inlog.getFilePtr());

            int percent = 100.0 * offset / (logSize == 0 ? 1 : logSize);
            if (percent != lastPrintPercent) {
                cout << "\r" << "Percent Complete: " << percent << flush;
                lastPrintPercent = percent;

                progress_dialog.setValue(percent);
                if (progress_dialog.wasCanceled()) {
                  interrupted = true;
                  break;
                }
            }

            evt = inlog.readNextEvent();
            if (evt == nullptr) break;

            processEvent(evt);
        }
        if (lastPrintPercent != 100 && !interrupted)
            cout << "\r" << "Percent Complete: 100" << flush;
        cout << endl;

        if (interrupted) progress_dialog.cancel();
    };

    processLog(processEvent);

    inlog.close();

    return 0;
}

bool DataLoadZcm::launchDialog(const string& filepath, unordered_set<string>& channels)
{
  QSettings settings;
  _dialog->restoreGeometry(settings.value("DataLoadZcm.geometry").toByteArray());


  channels.clear();
  auto processEvent = [&channels](const zcm::LogEvent* evt){
    channels.insert(evt->channel);
  };


  if (processInputLog(filepath, processEvent) != 0) {
    return false;
  }

  _ui->listWidgetChannels->clear();
  for (auto& c : channels) {
    auto chan = QString::fromStdString(c);
    _ui->listWidgetChannels->addItem(chan);
  }
  _ui->listWidgetChannels->sortItems();

  auto selected_channels = settings.value("DataLoadZcm.selected_channels").toStringList();
  for (int row = 0; row<_ui->listWidgetChannels->count(); row++) {
    auto item = _ui->listWidgetChannels->item(row);
    if(selected_channels.contains(item->text())) {
        item->setSelected(true);
    }
  }
  channels.clear();
  selected_channels.clear();

  int res = _dialog->exec();
  settings.setValue("DataLoadZcm.geometry", _dialog->saveGeometry());

  if (res == QDialog::Rejected) {
    return false;
  }

  QModelIndexList indexes = _ui->listWidgetChannels->selectionModel()->selectedRows();
  for (auto& i : indexes) {
    auto item = _ui->listWidgetChannels->item(i.row());
    channels.insert(item->text().toStdString());
    selected_channels.push_back(item->text());
  }

  settings.setValue("DataLoadZcm.selected_channels", selected_channels);

  return !indexes.empty();
}

template <typename T>
double toDouble(const void* data) {
  return static_cast<double>(*reinterpret_cast<const T*>(data));
}

bool DataLoadZcm::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  string filepath = info->filename.toStdString();
  unordered_set<string> channels;

  if (info->plugin_config.hasChildNodes()) {
    xmlLoadState(info->plugin_config.firstChildElement());
  } else {
    if (!launchDialog(filepath, channels)) {
      return false;
    }
  }

  zcm::TypeDb types(_config_widget->getLibraries().toStdString());
  if(!types.good())
  {
    QMessageBox::warning(nullptr, "Error", "Failed to load zcmtypes");
    return false;
  }

  vector<pair<string, double>> numerics;
  vector<pair<string, string>> strings;

  auto processData = [&](const string& name, zcm_field_type_t type, const void* data){
    switch (type) {
    case ZCM_FIELD_INT8_T: numerics.emplace_back(name, toDouble<int8_t>(data)); break;
    case ZCM_FIELD_INT16_T: numerics.emplace_back(name, toDouble<int16_t>(data)); break;
    case ZCM_FIELD_INT32_T: numerics.emplace_back(name, toDouble<int32_t>(data)); break;
    case ZCM_FIELD_INT64_T: numerics.emplace_back(name, toDouble<int64_t>(data)); break;
    case ZCM_FIELD_BYTE: numerics.emplace_back(name, toDouble<uint8_t>(data)); break;
    case ZCM_FIELD_FLOAT: numerics.emplace_back(name, toDouble<float>(data)); break;
    case ZCM_FIELD_DOUBLE: numerics.emplace_back(name, toDouble<double>(data)); break;
    case ZCM_FIELD_BOOLEAN: numerics.emplace_back(name, toDouble<bool>(data)); break;
    case ZCM_FIELD_STRING: strings.emplace_back(name, string((const char*)data)); break;
    case ZCM_FIELD_USER_TYPE: assert(false && "Should not be possble");
    }
  };

  auto processEvent = [&](const zcm::LogEvent* evt){
      if (channels.find(evt->channel) == channels.end()) return;
      zcm::Introspection::processEncodedType(evt->channel,
                                             evt->data, evt->datalen,
                                             "/",
                                             types, processData);

      /*
      for (auto& n : numerics) cout << setprecision(10) << n.first << "," << n.second << endl;
      for (auto& s : strings) cout << s.first << "," << s.second << endl;
      */

      for (auto& n : numerics) {
          auto itr = plot_data.numeric.find(n.first);
          if (itr == plot_data.numeric.end()) itr = plot_data.addNumeric(n.first);
          itr->second.pushBack({ (double)evt->timestamp / 1e6, n.second });
      }
      for (auto& s : strings) {
          auto itr = plot_data.strings.find(s.first);
          if (itr == plot_data.strings.end()) itr = plot_data.addStringSeries(s.first);
          itr->second.pushBack({ (double)evt->timestamp / 1e6, s.second });
      }

      numerics.clear();
      strings.clear();
  };

  if (processInputLog(filepath, processEvent) != 0)
      return false;

  return true;
}

bool DataLoadZcm::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataLoadZcm::xmlLoadState(const QDomElement&)
{
  return true;
}

#include "widget_remote_load.hpp"
#include "ui_widget_remote_load.h"
#include <QTableWidgetItem>
#include <QSettings>

RemoteLoad::RemoteLoad(QWidget *parent) :
  QDialog(parent),
  ui(new Ui::RemoteLoad)
{
  ui->setupUi(this);
  ui->linePort->setValidator( new QIntValidator(1, 9999, this) );

  QHeaderView* header = ui->tableWidget->horizontalHeader();
  header->setSectionResizeMode(0, QHeaderView::Stretch);
  header->setSectionResizeMode(1, QHeaderView::Fixed);
  header->setSectionResizeMode(2, QHeaderView::Fixed);

  connect (ui->buttonBox, &QDialogButtonBox::accepted, this, &RemoteLoad::accepted);
  connect (ui->buttonBox, &QDialogButtonBox::rejected, this, &RemoteLoad::rejected);

  QSettings settings;
  settings.beginGroup("RemoteLoadWidget");

  ui->spinBox->setValue( settings.value("max_array_size", 500).toInt());

  if(settings.value("clamp_large_array_size", true).toBool())
  {
    ui->radioClamp->setChecked(true);
  }
  else {
    ui->radioSkip->setChecked(true);
  }
  ui->lineHost->setText( settings.value("host", "localhost").toString() );
  ui->linePort->setText( QString::number( settings.value("port", 1667).toInt()) );
}

RemoteLoad::~RemoteLoad()
{
  QSettings settings;
  settings.beginGroup("RemoteLoadWidget");
  settings.setValue("max_array_size", ui->spinBox->value() );
  settings.setValue("clamp_large_array_size", ui->radioClamp->isChecked() );
  settings.setValue("host", ui->lineHost->text() );
  settings.setValue("port", ui->linePort->text().toInt() );

  delete ui;
}

void RemoteLoad::enableTopics(bool enable)
{
  ui->widgetTopics->setEnabled(enable);
}

void RemoteLoad::setFileName(QString name)
{
  ui->lineFilename->setText(name);
}

void RemoteLoad::setChannels(const std::vector<mcap_api::ChannelInfo> &channels)
{
  ui->tableWidget->setRowCount(0);
  for(const auto& channel: channels)
  {
    ui->tableWidget->insertRow(0);

    auto topic_name = new QTableWidgetItem(QString::fromStdString(channel.topic));
    ui->tableWidget->setItem(0, 0, topic_name);

    auto schema_encoding = new QTableWidgetItem(QString::fromStdString(channel.schema_encoding));
    ui->tableWidget->setItem(0, 1, schema_encoding);

    auto msg_count = new QTableWidgetItem(QString::number(channel.message_count));
    ui->tableWidget->setItem(0, 2, msg_count);
  }

  ui->tableWidget->sortByColumn(0);
  ui->tableWidget->resizeColumnToContents(1);
  ui->tableWidget->resizeColumnToContents(2);
}

void RemoteLoad::clear()
{
  ui->lineFilename->clear();
  ui->widgetTopics->setEnabled(false);
  ui->tableWidget->setRowCount(0);
}

bool RemoteLoad::clampLargeArrays() const
{
  return ui->radioClamp->isChecked();
}

int RemoteLoad::maxArraySize() const
{
  return ui->spinBox->value();
}

void RemoteLoad::setMaxTimeRange(double max_secs)
{
  ui->timeMin->setRange(0, max_secs);
  ui->timeMax->setRange(0, max_secs);
  ui->timeMax->setValue(max_secs);
  ui->labelMaxTime->setText(QString("%1").arg( max_secs, 0, 'f', 2 ));
}

std::pair<double, double> RemoteLoad::getTimeRange()
{
  if(ui->timeMin->value() > ui->timeMax->value())
  {
    double tmp = ui->timeMin->value();
    ui->timeMin->setValue(ui->timeMax->value());
    ui->timeMax->setValue(tmp);
  }
  return {ui->timeMin->value(), ui->timeMax->value()};
}

void RemoteLoad::on_buttonOpenFile_clicked()
{
  QString host = ui->lineHost->text();
  int port = ui->linePort->text().toInt();
  emit openFile(host, port);
}


void RemoteLoad::on_buttonLoadData_clicked()
{
  QStringList topics;
  QItemSelectionModel *select = ui->tableWidget->selectionModel();
  QModelIndexList rows = select->selectedRows();
  for(auto const& index: rows)
  {
    topics.push_back( ui->tableWidget->item(index.row(), 0)->text());
  }
  if(!topics.empty())
  {
    emit loadTopics(topics);
  }
}


void RemoteLoad::on_buttonResetTimes_clicked()
{
  ui->timeMin->setValue(ui->timeMax->minimum());
  ui->timeMax->setValue(ui->timeMax->maximum());
}


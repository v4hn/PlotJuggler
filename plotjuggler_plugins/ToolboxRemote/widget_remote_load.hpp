#ifndef WIDGET_REMOTE_LOAD_HPP
#define WIDGET_REMOTE_LOAD_HPP

#include <QDialog>
#include "mcap_api/mcap_api.h"

namespace Ui {
class RemoteLoad;
}

class RemoteLoad : public QDialog
{
  Q_OBJECT

public:
  explicit RemoteLoad(QWidget *parent = nullptr);
  ~RemoteLoad();

  void enableTopics(bool enable);

  void setFileName(QString name);

  void setChannels(const std::vector<mcap_api::ChannelInfo>& channles);

  void clear();

  bool clampLargeArrays() const;

  int maxArraySize() const;

  void setMaxTimeRange(double max_secs);

  std::pair<double, double> getTimeRange();

signals:

  void openFile(QString host, int port);
  void loadTopics(QStringList topics);
  void accepted();
  void rejected();

private slots:
  void on_buttonOpenFile_clicked();

  void on_buttonLoadData_clicked();

  void on_buttonResetTimes_clicked();

private:
  Ui::RemoteLoad *ui;

};

#endif // WIDGET_REMOTE_LOAD_HPP

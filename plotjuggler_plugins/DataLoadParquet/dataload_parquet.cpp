#include "dataload_parquet.h"
#include <QTextStream>
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QSettings>
#include <QProgressDialog>
#include <QDateTime>
#include <QInputDialog>
#include <QListWidget>
#include <cmath>

DataLoadParquet::DataLoadParquet()
{
  ui = new Ui::DialogParquet();
  _dialog = new QDialog();

  ui->setupUi(_dialog);

  connect(ui->checkBoxDateFormat, &QCheckBox::toggled, this,
          [=](bool checked) { ui->lineEditDateFormat->setEnabled(checked); });

  connect(ui->listWidgetSeries, &QListWidget::currentTextChanged, this,
          [=](QString text) { ui->buttonBox->setEnabled(!text.isEmpty()); });

  connect(ui->listWidgetSeries, &QListWidget::doubleClicked, this,
          [=](const QModelIndex&) { _dialog->accept(); });

  connect(ui->radioButtonIndex, &QRadioButton::toggled, this, [=](bool checked) {
    ui->buttonBox->setEnabled(checked);
    ui->listWidgetSeries->setEnabled(!checked);
  });

  QSettings settings;

  bool radio_index_checked = settings.value("DataLoadParquet::radioIndexChecked", false).toBool();
  ui->radioButtonIndex->setChecked(radio_index_checked);

  bool parse_date_time = settings.value("DataLoadParquet::parseDateTime", false).toBool();
  ui->checkBoxDateFormat->setChecked(parse_date_time);

  QString date_format = settings.value("DataLoadParquet::dateFromat", "").toString();
  if (date_format.isEmpty() == false)
  {
    ui->lineEditDateFormat->setText(date_format);
  }
}

DataLoadParquet::~DataLoadParquet()
{
  delete ui;
}

const std::vector<const char*>& DataLoadParquet::compatibleFileExtensions() const
{
  static std::vector<const char*> extensions = { "parquet" };
  return extensions;
}

template <typename T>
double get_numeric_value(parquet::StreamReader& os)
{
  parquet::StreamReader::optional<T> tmp;
  os >> tmp;
  return tmp ? static_cast<double>(*tmp) : std::numeric_limits<double>::quiet_NaN();
}

bool DataLoadParquet::readDataFromFile(FileLoadInfo* info, PlotDataMapRef& plot_data)
{
  using parquet::ConvertedType;
  using parquet::Type;

  parquet_reader_ = parquet::ParquetFileReader::OpenFile(info->filename.toStdString());

  if (!parquet_reader_)
  {
    return false;
  }

  std::shared_ptr<parquet::FileMetaData> file_metadata = parquet_reader_->metadata();
  const auto schema = file_metadata->schema();
  const size_t num_columns = file_metadata->num_columns();

  struct ColumnInfo
  {
    std::string name;
    parquet::Type::type column_type;
    parquet::ConvertedType::type converted_column_type;
    bool is_valid;
    PlotData* plot_data = nullptr;
  };

  std::vector<ColumnInfo> columns_info(num_columns);

  for (size_t col = 0; col < num_columns; col++)
  {
    const auto column = schema->Column(col);
    ColumnInfo info;
    info.name = column->name();
    info.column_type = column->physical_type();
    info.converted_column_type = column->converted_type();

    info.is_valid = (info.column_type == Type::BOOLEAN || info.column_type == Type::INT32 ||
                     info.column_type == Type::INT64 || info.column_type == Type::FLOAT ||
                     info.column_type == Type::DOUBLE);

    if (info.is_valid)
    {
      info.plot_data = &plot_data.getOrCreateNumeric(info.name, nullptr);
    }

    columns_info[col] = info;
    ui->listWidgetSeries->addItem(QString::fromStdString(column->name()));
  }

  {
    QSettings settings;
    if (_default_time_axis.isEmpty())
    {
      _default_time_axis = settings.value("DataLoadParquet::prevTimestamp", {}).toString();
    }

    if (_default_time_axis.isEmpty() == false)
    {
      auto items = ui->listWidgetSeries->findItems(_default_time_axis, Qt::MatchExactly);
      if (items.size() > 0)
      {
        ui->listWidgetSeries->setCurrentItem(items.front());
      }
    }
  }

  int ret = _dialog->exec();
  if (ret != QDialog::Accepted)
  {
    return false;
  }

  QString selected_stamp;

  if (ui->radioButtonSelect->isChecked())
  {
    auto selected = ui->listWidgetSeries->selectedItems();
    if (selected.size() == 1)
    {
      selected_stamp = selected.front()->text();
    }
  }

  QSettings settings;
  settings.setValue("DataLoadParquet::prevTimestamp", selected_stamp);
  settings.setValue("DataLoadParquet::radioIndexChecked", ui->radioButtonIndex->isChecked());
  settings.setValue("DataLoadParquet::parseDateTime", ui->checkBoxDateFormat->isChecked());
  settings.setValue("DataLoadParquet::dateFromat", ui->lineEditDateFormat->text());

  //-----------------------------
  // Time to parse
  int timestamp_column = -1;

  for (size_t col = 0; col < num_columns; col++)
  {
    if (columns_info[col].name == selected_stamp.toStdString())
    {
      timestamp_column = col;
      break;
    }
  }

  QProgressDialog progress_dialog;
  progress_dialog.setWindowTitle("Loading the Parquet file");
  progress_dialog.setLabelText("Loading... please wait");
  progress_dialog.setWindowModality(Qt::ApplicationModal);
  progress_dialog.setRange(0, parquet_reader_->metadata()->num_rows());
  progress_dialog.setAutoClose(true);
  progress_dialog.setAutoReset(true);
  progress_dialog.show();

  parquet::StreamReader os{ std::move(parquet_reader_) };

  constexpr auto NaN = std::numeric_limits<double>::quiet_NaN();
  std::vector<double> row_values(num_columns, NaN);

  int row = 0;

  while (!os.eof())
  {
    // extract an entire row
    for (size_t col = 0; col < num_columns; col++)
    {
      const auto& info = columns_info[col];
      if (!info.is_valid)
      {
        os.SkipColumns(1);
        continue;
      }

      switch (info.column_type)
      {
        case Type::BOOLEAN: {
          row_values[col] = get_numeric_value<bool>(os);
          break;
        }
        case Type::INT32:
        case Type::INT64: {
          switch (info.converted_column_type)
          {
            case ConvertedType::INT_8: {
              row_values[col] = get_numeric_value<int8_t>(os);
              break;
            }
            case ConvertedType::INT_16: {
              row_values[col] = get_numeric_value<int16_t>(os);
              break;
            }
            case ConvertedType::INT_32: {
              row_values[col] = get_numeric_value<int32_t>(os);
              break;
            }
            case ConvertedType::INT_64: {
              row_values[col] = get_numeric_value<int64_t>(os);
              break;
            }
            case ConvertedType::UINT_8: {
              row_values[col] = get_numeric_value<uint8_t>(os);
              break;
            }
            case ConvertedType::UINT_16: {
              row_values[col] = get_numeric_value<uint16_t>(os);
              break;
            }
            case ConvertedType::UINT_32: {
              row_values[col] = get_numeric_value<uint32_t>(os);
              break;
            }
            case ConvertedType::UINT_64: {
              row_values[col] = get_numeric_value<uint64_t>(os);
              break;
            }
            default: {
              // Fallback in case no converted type is provided
              switch (info.column_type)
              {
                case Type::INT32: {
                  row_values[col] = get_numeric_value<int32_t>(os);
                  break;
                }
                case Type::INT64: {
                  row_values[col] = get_numeric_value<int64_t>(os);
                  break;
                }
              }
            }
          }
          break;
        }
        case Type::FLOAT: {
          row_values[col] = get_numeric_value<float>(os);
          break;
        }
        case Type::DOUBLE: {
          row_values[col] = get_numeric_value<double>(os);
          break;
        }
        default: {
        }
      }  // end switch
    }    // end for column

    os >> parquet::EndRow;

    double timestamp = timestamp_column >= 0 ? row_values[timestamp_column] : row;
    row++;

    for (size_t col = 0; col < num_columns; col++)
    {
      auto& info = columns_info[col];
      const auto& value = row_values[col];
      if (info.is_valid && !std::isnan(value))
      {
        info.plot_data->pushBack({ timestamp, value });
      }
    }

    if (row % 100 == 0)
    {
      progress_dialog.setValue(row);
      QApplication::processEvents();
      if (progress_dialog.wasCanceled())
      {
        break;
      }
    }
  }

  return true;
}

bool DataLoadParquet::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  QDomElement elem = doc.createElement("default");
  elem.setAttribute("prevTimestamp", _default_time_axis);
  elem.setAttribute("radioIndexChecked", ui->radioButtonIndex->isChecked());
  elem.setAttribute("parseDateTime", ui->checkBoxDateFormat->isChecked());
  elem.setAttribute("dateFromat", ui->lineEditDateFormat->text());

  parent_element.appendChild(elem);
  return true;
}

bool DataLoadParquet::xmlLoadState(const QDomElement& parent_element)
{
  QDomElement elem = parent_element.firstChildElement("default");
  if (!elem.isNull())
  {
    if (elem.hasAttribute("prevTimestamp"))
    {
      _default_time_axis = elem.attribute("prevTimestamp");
    }
    if (elem.hasAttribute("radioIndexChecked"))
    {
      bool checked = elem.attribute("radioIndexChecked").toInt();
      ui->radioButtonIndex->setChecked(checked);
    }
    if (elem.hasAttribute("parseDateTime"))
    {
      bool checked = elem.attribute("parseDateTime").toInt();
      ui->checkBoxDateFormat->setChecked(checked);
    }
    if (elem.hasAttribute("dateFromat"))
    {
      ui->lineEditDateFormat->setText(elem.attribute("dateFromat"));
    }
  }

  return true;
}

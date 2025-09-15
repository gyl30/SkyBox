#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedLayout>
#include <QFileInfo>
#include <QLocale>

#include "gui/util.h"
#include "gui/file_item_widget.h"

file_item_widget::file_item_widget(QWidget *parent) : QWidget(parent)
{
    setup_ui();
    setAttribute(Qt::WA_TranslucentBackground);
}

void file_item_widget::setup_ui()
{
    auto *main_layout = new QGridLayout(this);
    main_layout->setContentsMargins(15, 0, 15, 0);
    main_layout->setSpacing(10);
    main_layout->setAlignment(Qt::AlignVCenter);

    auto *file_info_widget = new QWidget();
    auto *file_info_layout = new QVBoxLayout(file_info_widget);
    file_info_layout->setContentsMargins(0, 0, 0, 0);
    file_info_layout->setSpacing(2);

    file_name_label_ = new QLabel("File Name Placeholder");
    file_name_label_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);

    size_label_ = new QLabel("0 KB / 0 KB");
    file_info_layout->addWidget(file_name_label_);
    file_info_layout->addWidget(size_label_);

    auto *progress_widget = new QWidget();
    auto *progress_layout = new QGridLayout(progress_widget);
    progress_layout->setContentsMargins(0, 0, 10, 0);
    progress_layout->setVerticalSpacing(2);
    progress_layout->setHorizontalSpacing(10);

    progress_bar_ = new QProgressBar();
    progress_bar_->setFixedHeight(22);
    progress_bar_->setTextVisible(true);
    progress_bar_->setAlignment(Qt::AlignCenter);

    time_label_ = new QLabel("00:00");

    progress_layout->addWidget(progress_bar_, 0, 0, 1, 2);
    progress_layout->addWidget(time_label_, 1, 0);

    progress_layout->setColumnStretch(0, 1);
    progress_layout->setColumnStretch(1, 0);

    speed_label_ = new QLabel("0.0 MB/s");
    speed_label_->setAlignment(Qt::AlignCenter);

    auto *actions_widget = new QWidget();
    auto *actions_layout = new QHBoxLayout(actions_widget);
    actions_layout->setContentsMargins(0, 0, 0, 0);
    actions_layout->setSpacing(15);
    action_button_ = new QPushButton();
    cancel_button_ = new QPushButton();
    action_button_->setFixedSize(22, 22);
    cancel_button_->setFixedSize(22, 22);
    action_button_->setFlat(true);
    cancel_button_->setFlat(true);
    actions_layout->addWidget(action_button_);
    actions_layout->addWidget(cancel_button_);

    main_layout->addWidget(file_info_widget, 0, 0);
    main_layout->addWidget(progress_widget, 0, 1);
    main_layout->addWidget(speed_label_, 0, 2);
    main_layout->addWidget(actions_widget, 0, 3);

    main_layout->setColumnStretch(0, 5);
    main_layout->setColumnStretch(1, 3);
    main_layout->setColumnStretch(2, 1);
    main_layout->setColumnStretch(3, 1);
}

void file_item_widget::set_data(const leaf::file_event &task)
{
    file_name_label_->setText(QFileInfo(QString::fromStdString(task.filename)).fileName());
    file_name_label_->setToolTip(file_name_label_->text());

    size_label_->setText(
        QString("%1 / %2")
            .arg(QLocale().formattedDataSize(static_cast<int64_t>(task.process_size + task.offset), 1, QLocale::DataSizeTraditionalFormat))
            .arg(QLocale().formattedDataSize(static_cast<int64_t>(task.file_size), 1, QLocale::DataSizeTraditionalFormat)));

    int percentage = (task.file_size > 0) ? static_cast<int>((task.process_size * 100) / task.file_size) : 0;
    progress_bar_->setValue(percentage);

    if (task.file_size > 0)
    {
        progress_bar_->setFormat(
            QString::asprintf("%.1f%%", static_cast<float>((task.process_size + task.offset) * 100) / static_cast<float>(task.file_size)));
    }
    else
    {
        progress_bar_->setFormat("0.0%");
    }

    int second = 0;
    if (task.remaining_time_mil > 1000)
    {
        second = static_cast<int>(task.remaining_time_mil / 1000);
    }
    time_label_->setText(leaf::format_time(second));
    speed_label_->setText(QString::asprintf("%.1d KB/s", 0));
    if (task.process_size > 0 && task.remaining_time_mil > 0)
    {
        auto bytes_per_sec = static_cast<double>(task.process_size / task.remaining_time_mil);    // NOLINT
        bytes_per_sec = bytes_per_sec * 1000;
        if (bytes_per_sec >= 1024 * 1024)
        {
            auto speed_value = bytes_per_sec / (1024.0 * 1024.0);
            speed_label_->setText(QString::asprintf("%.1f MB/s", speed_value));
        }
        else
        {
            auto speed_value = bytes_per_sec / 1024.0;
            speed_label_->setText(QString::asprintf("%.1f KB/s", speed_value));
        }
    }
}

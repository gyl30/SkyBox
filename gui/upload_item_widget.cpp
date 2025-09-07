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
#include "gui/upload_item_widget.h"

upload_item_widget::upload_item_widget(QWidget *parent) : QWidget(parent)
{
    setup_ui();
    setAttribute(Qt::WA_TranslucentBackground);
}

void upload_item_widget::setup_ui()
{
    auto *main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(15, 0, 15, 0);
    main_layout->setSpacing(20);
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
    progress_layout->setContentsMargins(0, 0, 45, 0);
    progress_layout->setVerticalSpacing(2);
    progress_layout->setHorizontalSpacing(45);

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
    actions_layout->setSpacing(45);
    action_button_ = new QPushButton();
    cancel_button_ = new QPushButton();
    action_button_->setFixedSize(22, 22);
    cancel_button_->setFixedSize(22, 22);
    action_button_->setFlat(true);
    cancel_button_->setFlat(true);
    actions_layout->addWidget(action_button_);
    actions_layout->addWidget(cancel_button_);

    main_layout->addWidget(file_info_widget);
    main_layout->addWidget(progress_widget);
    main_layout->addWidget(speed_label_);
    main_layout->addSpacing(45);
    main_layout->addWidget(actions_widget);

    main_layout->setStretch(0, 5);
    main_layout->setStretch(1, 3);
    main_layout->setStretch(2, 0);
    main_layout->setStretch(3, 0);
}

void upload_item_widget::set_data(const leaf::file_event &task)
{
    file_name_label_->setText(QFileInfo(QString::fromStdString(task.filename)).fileName());
    file_name_label_->setToolTip(file_name_label_->text());

    size_label_->setText(QString("%1 / %2")
                             .arg(QLocale().formattedDataSize(static_cast<int64_t>(task.process_size), 1, QLocale::DataSizeTraditionalFormat))
                             .arg(QLocale().formattedDataSize(static_cast<int64_t>(task.file_size), 1, QLocale::DataSizeTraditionalFormat)));

    int percentage = (task.file_size > 0) ? static_cast<int>((task.process_size * 100) / task.file_size) : 0;
    progress_bar_->setValue(percentage);

    if (task.file_size > 0)
    {
        progress_bar_->setFormat(QString::asprintf("%.1f%%", static_cast<float>(task.process_size) / static_cast<float>(task.file_size)));
    }
    else
    {
        progress_bar_->setFormat("0.0%");
    }

    time_label_->setText(leaf::format_time(task.remaining_time_sec));
    speed_label_->setText(QString::asprintf("%.1f MB/s", task.speed_mbps));
}

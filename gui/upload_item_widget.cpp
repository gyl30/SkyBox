#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStyle>
#include <QIcon>
#include <QPainter>
#include <QLocale>
#include <QFontMetrics>
#include <QFileInfo>
#include <utility>
#include "gui/util.h"
#include "gui/upload_item_widget.h"

upload_item_widget::upload_item_widget(leaf::upload_event e, QWidget *parent) : QWidget(parent), event_(std::move(e)) { setup_ui(); }

void upload_item_widget::setup_ui()
{
    auto *main_layout = new QHBoxLayout(this);
    main_layout->setContentsMargins(5, 3, 5, 3);
    main_layout->setSpacing(8);
    // filename
    filename_label_ = new QLabel();
    main_layout->addWidget(filename_label_, 6);
    // progress bar
    progress_bar_ = new QProgressBar();
    progress_bar_->setFixedHeight(12);
    progress_bar_->setTextVisible(true);
    progress_bar_->setFormat("%p%");
    progress_bar_->setAlignment(Qt::AlignCenter);
    main_layout->addWidget(progress_bar_, 3);

    action_button_ = new QPushButton();
    action_button_->setFlat(true);

    cancel_button_ = new QPushButton();
    cancel_button_->setFlat(true);
    cancel_button_->setToolTip("取消");

    main_layout->addWidget(action_button_, 1);
    main_layout->addWidget(cancel_button_, 1);

    this->setLayout(main_layout);
    connect(action_button_, &QPushButton::clicked, this, &upload_item_widget::handle_action_button_click);
    connect(cancel_button_, &QPushButton::clicked, this, &upload_item_widget::handle_cancel_button_click);
    action_button_->setIcon(leaf::emoji_to_icon("⏸️", 64));
    action_button_->setToolTip("暂停");
    cancel_button_->setIcon(leaf::emoji_to_icon("❌", 64));
}

void upload_item_widget::update(const leaf::upload_event &e)
{
    event_ = e;
    int percentage = (e.file_size > 0) ? static_cast<int>((e.upload_size * 100) / e.file_size) : 0;
    progress_bar_->setValue(percentage);
    QString filename = QFileInfo(QString::fromStdString(e.filename)).fileName();

    QFontMetrics metrics(filename_label_->font());
    QString elided_text = metrics.elidedText(filename, Qt::ElideRight, filename_label_->width());
    filename_label_->setText(elided_text);
    filename_label_->setToolTip(filename);
}

QSize upload_item_widget::sizeHint() const { return {(parentWidget() != nullptr) ? parentWidget()->width() - 20 : 300, 60}; }
void upload_item_widget::pause()
{
    action_button_->setIcon(leaf::emoji_to_icon("▶️", 64));
    QString tip = event_.upload_size == 0 ? "开始" : "继续";
    action_button_->setToolTip(tip);
}

void upload_item_widget::cacnel() {}

void upload_item_widget::handle_action_button_click() {}

void upload_item_widget::handle_cancel_button_click() {}

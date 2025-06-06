#include <QPainter>
#include <QStyleOption>
#include "gui/util.h"
#include "gui/titlebar.h"

namespace leaf
{

title_bar::title_bar(QWidget *parent) : QWidget(parent)
{
    setFixedHeight(40);
    setObjectName("TitleBar");

    title_label_ = new QLabel("文件传输", this);
    title_label_->setObjectName("TitleLabel");
    title_label_->setAlignment(Qt::AlignCenter);

    min_btn_ = new QPushButton(this);
    min_btn_->setObjectName("MinButton");
    min_btn_->setFixedSize(30, 30);
    min_btn_->setToolTip("最小化");

    close_btn_ = new QPushButton(this);
    close_btn_->setObjectName("CloseButton");
    close_btn_->setFixedSize(30, 30);
    close_btn_->setToolTip("关闭");

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 8, 0);
    layout->setSpacing(10);
    layout->addWidget(title_label_);
    layout->addStretch();
    layout->addWidget(min_btn_);
    layout->addWidget(close_btn_);

    setStyleSheet(R"(
        QPushButton {
            background: transparent;
            border: none;
            border-radius: 0;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.2);
        }
        QPushButton#MinButton:hover {
            background: rgba(0, 0, 0, 0.2);
        }
        QPushButton#CloseButton:hover {
            background: #ff4c4c;
        }
        QLabel#TitleLabel {
            font-weight: bold;
            padding-left: 0px;
        }
    )");

    // 设置按钮图标
    min_btn_->setIcon(leaf::emoji_to_icon("➖", 24));
    close_btn_->setIcon(leaf::emoji_to_icon("❌", 24));
    min_btn_->setFixedSize(36, 36);
    close_btn_->setFixedSize(36, 36);
    min_btn_->setIconSize(QSize(24, 24));
    close_btn_->setIconSize(QSize(24, 24));

    connect(min_btn_, &QPushButton::clicked, this, &title_bar::minimizeClicked);
    connect(close_btn_, &QPushButton::clicked, this, &title_bar::closeClicked);
}

void title_bar::mousePressEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        drag_position_ = event->globalPosition().toPoint() - parentWidget()->frameGeometry().topLeft();
        event->accept();
    }
}

void title_bar::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        parentWidget()->move(event->globalPosition().toPoint() - drag_position_);
        event->accept();
    }
}
}    // namespace leaf

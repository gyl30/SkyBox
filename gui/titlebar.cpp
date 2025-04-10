#include <QPainter>
#include "gui/titlebar.h"
#include <QStyleOption>

static QIcon emoji_to_icon(const QString &emoji, int size)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QFont font("EmojiOne");
    font.setPointSizeF(size * 0.5);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    painter.end();
    return pixmap;
}

TitleBar::TitleBar(QWidget *parent) : QWidget(parent)
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
        QWidget#TitleBar {
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
        }
        QPushButton {
            background: transparent;
            border: none;
            padding: 4px;
            min-width: 32px;
            min-height: 32px;
            border-radius: 0;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 0.2);
        }
        QPushButton#MinButton:hover {
            background: rgba(0, 0, 0, 0.2);
        }
        QPushButton#CloseButton:hover {
            background: #F44336;
        }
        QLabel#TitleLabel {
            font-size: 18px;
            font-weight: bold;
            padding-left: 0px;
        }
    )");

    // 设置按钮图标
    min_btn_->setIcon(emoji_to_icon("➖", 24));
    close_btn_->setIcon(emoji_to_icon("❌", 24));
    min_btn_->setIconSize(QSize(24, 24));
    close_btn_->setIconSize(QSize(24, 24));

    connect(min_btn_, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(close_btn_, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        drag_position_ = event->globalPos() - parentWidget()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        parentWidget()->move(event->globalPos() - drag_position_);
        event->accept();
    }
}

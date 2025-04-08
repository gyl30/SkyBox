#include <QPainter>
#include "gui/titlebar.h"

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
    setFixedHeight(60);

    title_label_ = new QLabel(this);
    title_label_->setStyleSheet("background: transparent;");

    min_btn_ = new QPushButton(this);
    min_btn_->setIcon(emoji_to_icon("➖", 40));
    min_btn_->setIconSize(QSize(40, 40));
    min_btn_->setStyleSheet("background:transparent;border:none;padding-right:10px;padding-left:10px;");

    close_btn_ = new QPushButton(this);
    close_btn_->setIcon(emoji_to_icon("❌", 40));
    close_btn_->setIconSize(QSize(40, 40));
    close_btn_->setStyleSheet("background:transparent;border:none;padding-right:10px;padding-left:10px;");

    auto *layout = new QHBoxLayout();
    layout->addWidget(title_label_);
    layout->addStretch();
    layout->addWidget(min_btn_);
    layout->addWidget(close_btn_);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    setLayout(layout);

    connect(min_btn_, &QPushButton::clicked, this, &TitleBar::minimizeClicked);
    connect(close_btn_, &QPushButton::clicked, this, &TitleBar::closeClicked);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        drag_position_ = event->globalPos() - parentWidget()->frameGeometry().topLeft();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        parentWidget()->move(event->globalPos() - drag_position_);
    }
}

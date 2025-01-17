#include <QPainter>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QStyleOptionProgressBar>
#include "gui/files_delegate.h"

namespace leaf
{

files_delegate::files_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize files_delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QStyledItemDelegate::sizeHint(option, index);
    }
    auto text = index.data(Qt::DisplayRole).toString();
    auto fond = QApplication::font();
    auto font_metrics = QFontMetrics(fond);
    int flag = Qt::AlignLeft | Qt::AlignTop | Qt::TextWrapAnywhere;
    auto rect = font_metrics.boundingRect(0, 0, option.rect.width(), 0, flag, text);
    auto size = QSize(option.rect.width(), option.rect.height() + rect.height());
    return size;
}

void files_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    painter->save();

    if ((option.state & QStyle::State_MouseOver) != 0U)
    {
        painter->fillRect(option.rect, option.palette.shadow());
    }
    if ((option.state & QStyle::State_Selected) != 0U)
    {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    auto mode = QIcon::Normal;
    auto state = QIcon::Off;
    if ((option.state & QStyle::State_Open) != 0U)
    {
        state = QIcon::On;
    }
    auto icon = index.data(Qt::DecorationRole).value<QIcon>();
    auto icon_rect = option.rect.adjusted(0, 6, 0, 0);
    icon_rect.setSize(QSize(option.rect.width(), 40));
    icon.paint(painter, icon_rect, Qt::AlignCenter | Qt::AlignVCenter, mode, state);
    auto text = index.data(Qt::DisplayRole).toString();
    auto text_rect = option.rect.adjusted(4, 45, -12, -4);
    int flag = Qt::AlignHCenter | Qt::AlignTop | Qt::TextWrapAnywhere;
    painter->drawText(text_rect, flag, option.fontMetrics.elidedText(text, Qt::ElideRight, option.rect.width() * 1.5));
    painter->restore();
}

}    // namespace leaf

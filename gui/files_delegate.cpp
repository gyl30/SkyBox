#include <QPainter>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QStyleOptionProgressBar>
#include "log/log.h"
#include "gui/gfile.h"
#include "gui/files_delegate.h"

namespace leaf
{

files_delegate::files_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

QSize files_delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    return QStyledItemDelegate::sizeHint(option, index);
}

void files_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }
    painter->save();

    auto gf = index.data(Qt::UserRole).value<leaf::gfile>();
    QStyleOptionButton button;
    if (gf.type == "dir")
    {
        button.icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    }
    else
    {
        button.icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    }
    LOG_DEBUG("paint filename {} type {}", gf.filename, gf.type);
    button.rect = option.rect;
    // button.text = QString::fromStdString(gf.filename);
    button.iconSize = button.rect.size() * 0.8;
    button.state = QStyle::State_Enabled;
    QApplication::style()->drawControl(QStyle::CE_PushButton, &button, painter);
    painter->restore();
}

}    // namespace leaf

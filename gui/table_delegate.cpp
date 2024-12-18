#include <QPainter>
#include <QApplication>
#include <QStyledItemDelegate>
#include <QStyleOptionProgressBar>
#include "gui/table_delegate.h"

namespace leaf
{

task_style_delegate::task_style_delegate(QObject *parent) : QStyledItemDelegate(parent) {}

void task_style_delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QStyledItemDelegate::paint(painter, option, index);
    }
    painter->save();

    const QStyle *style(QApplication::style());

    QStyleOptionProgressBar p;
    p.state = option.state | QStyle::State_Horizontal | QStyle::State_Small;
    p.direction = QApplication::layoutDirection();
    p.rect = option.rect;
    p.rect.moveCenter(option.rect.center());
    p.minimum = 0;
    p.maximum = 100;
    p.textAlignment = Qt::AlignCenter;
    p.textVisible = true;
    p.progress = index.model()->data(index, Qt::DisplayRole).toInt();
    p.text = QStringLiteral("处理中 %1%").arg(p.progress);
    if (p.progress >= 100)
    {
        p.text = "处理完成";
    }
    if (p.progress == 0)
    {
        p.text = "等待中";
    }
    style->drawControl(QStyle::CE_ProgressBar, &p, painter);
    painter->restore();
}

}    // namespace leaf

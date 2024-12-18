#ifndef LEAF_GUI_TABLE_DELEGATE_H
#define LEAF_GUI_TABLE_DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

namespace leaf
{
class task_style_delegate : public QStyledItemDelegate
{
   public:
    explicit task_style_delegate(QObject *parent = nullptr);

   public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const override
    {
        return QSize(option.rect.size());
    }
};

}    // namespace leaf

#endif

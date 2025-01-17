#ifndef LEAF_GUI_FILES_DELEGATE_H
#define LEAF_GUI_FILES_DELEGATE_H

#include <QObject>
#include <QStyledItemDelegate>

namespace leaf
{
class files_delegate : public QStyledItemDelegate
{
   public:
    explicit files_delegate(QObject *parent = nullptr);

   public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex & /*index*/) const override;
};

}    // namespace leaf

#endif

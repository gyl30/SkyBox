#ifndef LEAF_GUI_UPLOAD_ITEM_DELEGATE_H
#define LEAF_GUI_UPLOAD_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class upload_item_delegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit upload_item_delegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override;

   signals:
    void pause_button_clicked(const QModelIndex &index);
    void cancel_button_clicked(const QModelIndex &index);
};

#endif

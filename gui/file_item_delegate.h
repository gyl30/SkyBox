#ifndef LEAF_GUI_FILE_ITEM_DELEGATE_H
#define LEAF_GUI_FILE_ITEM_DELEGATE_H

#include <QModelIndex>
#include <QStyledItemDelegate>

class file_item_delegate : public QStyledItemDelegate
{
    Q_OBJECT

   public:
    explicit file_item_delegate(QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    void setEditorData(QWidget *editor, const QModelIndex &index) const override;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

   signals:
    void pause_button_clicked(const QModelIndex &index) const;
    void cancel_button_clicked(const QModelIndex &index) const;
};

#endif

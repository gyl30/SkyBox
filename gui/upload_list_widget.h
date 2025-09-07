#ifndef LEAF_GUI_UPLOAD_LIST_WIDGET_H
#define LEAF_GUI_UPLOAD_LIST_WIDGET_H

#include <QWidget>
#include "file/event.h"

class QListView;
class file_task_model;
class file_item_delegate;


class file_list_widget : public QWidget
{
    Q_OBJECT

   public:
    explicit file_list_widget(QWidget *parent = nullptr);
    ~file_list_widget() override = default;

   public slots:
    void add_task_to_view(const leaf::file_event &e);
    void remove_task_from_view(const leaf::file_event &e);

   private slots:
    void on_pause_button_clicked(const QModelIndex &index);
    void on_cancel_button_clicked(const QModelIndex &index);

   private:
    QListView *list_view_;
    file_task_model *model_;
    file_item_delegate *delegate_;
};

#endif

#ifndef LEAF_GUI_UPLOAD_LIST_WIDGET_H
#define LEAF_GUI_UPLOAD_LIST_WIDGET_H

#include <QWidget>
#include <QListView>
#include "file/event.h"
#include "gui/upload_task_model.h"
#include "gui/upload_item_delegate.h"

class upload_list_widget : public QWidget
{
    Q_OBJECT

   public:
    explicit upload_list_widget(QWidget *parent = nullptr);
    ~upload_list_widget() override = default;

   public slots:
    void add_task_to_view(const leaf::upload_event &e);
    void remove_task_from_view(const leaf::upload_event &e);

   private slots:
    void on_action_button_clicked(const QModelIndex &index);
    void on_cancel_button_clicked(const QModelIndex &index);

   private:
    QListView *list_view_;
    upload_task_model *model_;
    upload_item_delegate *delegate_;
};

#endif

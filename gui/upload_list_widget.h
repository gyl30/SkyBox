#ifndef LEAF_GUI_UPLOAD_LIST_WIDGET_H
#define LEAF_GUI_UPLOAD_LIST_WIDGET_H

#include <QWidget>
#include <QListWidget>
#include "file/event.h"

class upload_list_widget : public QWidget
{
    Q_OBJECT

   public:
    explicit upload_list_widget(QWidget *parent = nullptr);

   public:
    void add_task_to_view(const leaf::upload_event &e);
    void remove_task_from_view(const leaf::upload_event &e);

   private:
    QListWidget *list_widget_;
};

#endif

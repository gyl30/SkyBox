#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "table_view.h"
#include "table_model.h"

class Widget : public QWidget
{
    Q_OBJECT

   public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget() override;

   private:
    void on_new_file_clicked();
   private:
    leaf::task_model* model_ = nullptr;
    leaf::task_table_view* table_view_ = nullptr;
};
#endif    // WIDGET_H

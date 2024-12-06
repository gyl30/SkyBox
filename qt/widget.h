#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include "table_view.h"
#include "table_model.h"
#include "file_transfer_client.h"

class Widget : public QWidget
{
    Q_OBJECT

   public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget() override;

   private:
    void on_new_file_clicked();

   private:
    void upload_progress(const leaf::upload_event& e);
    void download_progress(const leaf::download_event& e);

   private:
    leaf::task_model* model_ = nullptr;
    leaf::task_table_view* table_view_ = nullptr;
    leaf::file_transfer_client* file_client_ = nullptr;
};
#endif    // WIDGET_H

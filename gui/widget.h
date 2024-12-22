#ifndef LEAF_GUI_WIDGET_H
#define LEAF_GUI_WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QTableWidget>
#include "gui/task.h"
#include "gui/table_view.h"
#include "gui/table_model.h"
#include "file/file_transfer_client.h"

class Widget : public QWidget
{
    Q_OBJECT

   public:
    explicit Widget(QWidget* parent = nullptr);
    ~Widget() override;

   private:
    void on_new_file_clicked();

   Q_SIGNALS:
    void progress_slot(leaf::task e);

   private Q_SLOTS:
    void on_progress_slot(leaf::task e);
    void on_login_slot(QString ip, QString port);

   private:
    void upload_progress(const leaf::upload_event& e);
    void download_progress(const leaf::download_event& e);
    void setting_btn_clicked();

   private:
    QPushButton* finish_btn_ = nullptr;
    QPushButton* progress_btn_ = nullptr;
    QPushButton* upload_btn_ = nullptr;
    QPushButton* setting_btn_ = nullptr;
    QStackedWidget* stacked_widget_ = nullptr;
    QTableWidget* finish_list_widget_ = nullptr;
    int finish_list_index_ = -1;
    int upload_list_index_ = -1;
    leaf::task_model* model_ = nullptr;
    leaf::task_table_view* table_view_ = nullptr;
    leaf::file_transfer_client* file_client_ = nullptr;
};
#endif    // WIDGET_H

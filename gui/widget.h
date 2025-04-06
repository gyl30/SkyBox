#ifndef LEAF_GUI_WIDGET_H
#define LEAF_GUI_WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QTableWidget>
#include <QStringList>
#include "gui/task.h"
#include "gui/table_view.h"
#include "gui/table_model.h"
#include "gui/files_widget.h"
#include "file/event.h"
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
    void progress_slot(const leaf::task& e);
    void notify_event_slot(const leaf::notify_event& e);

   private Q_SLOTS:
    void on_progress_slot(const leaf::task& e);
    void on_login_slot(const QString& user, const QString& passwd);
    void on_style_btn_clicked();
    void on_notify_event_slot(const leaf::notify_event& e);

   private:
    void notify_progress(const leaf::notify_event& e);
    void upload_progress(const leaf::upload_event& e);
    void download_progress(const leaf::download_event& e);
    void setting_btn_clicked();
    void on_files(const leaf::files_response& files);

   private:
    QToolButton* finish_btn_ = nullptr;
    QToolButton* progress_btn_ = nullptr;
    QToolButton* upload_btn_ = nullptr;
    QToolButton* login_btn_ = nullptr;
    QToolButton* files_btn_ = nullptr;
    QToolButton* style_btn_ = nullptr;
    QButtonGroup* btn_group_ = nullptr;
    QStackedWidget* stacked_widget_ = nullptr;
    QStringList style_list_;
    int style_index_ = 0;
    QTableWidget* finish_list_widget_ = nullptr;
    leaf::files_widget* files_widget_ = nullptr;
    int finish_list_index_ = -1;
    int upload_list_index_ = -1;
    int files_list_index_ = -1;
    leaf::task_model* model_ = nullptr;
    leaf::task_table_view* table_view_ = nullptr;
    leaf::file_transfer_client* file_client_ = nullptr;
};
#endif    // WIDGET_H

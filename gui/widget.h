#ifndef LEAF_GUI_WIDGET_H
#define LEAF_GUI_WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QTableWidget>
#include <QStringList>
#include <QLabel>
#include <QLineEdit>
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
    void error_occurred(const QString& error_msg);

   private Q_SLOTS:
    void on_progress_slot(const leaf::task& e);
    void on_style_btn_clicked();
    void on_notify_event_slot(const leaf::notify_event& e);
    void on_error_occurred(const QString& error_msg);
    void reset_ui_state();

   private:
    void notify_progress(const leaf::notify_event& e);
    void upload_progress(const leaf::upload_event& e);
    void download_progress(const leaf::download_event& e);
    void on_login_btn_clicked();
    void on_files(const leaf::files_response& files);
    void update_progress_btn_icon();
    void error_progress(const boost::system::error_code& ec);

   public:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;

   private:
    QToolButton* finish_btn_ = nullptr;
    QToolButton* progress_btn_ = nullptr;
    QToolButton* upload_btn_ = nullptr;
    QToolButton* login_btn_ = nullptr;
    QToolButton* files_btn_ = nullptr;
    QToolButton* style_btn_ = nullptr;
    QButtonGroup* btn_group_ = nullptr;
    QLabel* user_label_ = nullptr;
    QLineEdit* user_edit_ = nullptr;
    QLabel* key_label_ = nullptr;
    QLineEdit* key_edit_ = nullptr;
    QTimer* progress_timer_ = nullptr;
    size_t progress_frame_index_ = 0;
    const std::vector<QString> hourglass_frames_ = {"⌛", "⏳"};
    QStackedWidget* stacked_widget_ = nullptr;
    QStringList style_list_;
    int style_index_ = 0;
    QPoint click_pos_;
    QTableWidget* finish_list_widget_ = nullptr;
    leaf::files_widget* files_widget_ = nullptr;
    int finish_list_index_ = -1;
    int upload_list_index_ = -1;
    int files_list_index_ = -1;
    leaf::task_model* model_ = nullptr;
    leaf::task_table_view* table_view_ = nullptr;
    std::shared_ptr<leaf::file_transfer_client> file_client_ = nullptr;
    QMap<QString, QString> themes_;
    QStringList theme_names_;
    int32_t current_theme_index_{0};
};
#endif    // WIDGET_H

#ifndef LEAF_GUI_WIDGET_H
#define LEAF_GUI_WIDGET_H

#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QTableWidget>
#include <QStringList>
#include <QButtonGroup>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QListView>
#include <QTimer>
#include "file/event.h"
#include "file/directory.h"
#include "gui/file_model.h"
#include "gui/upload_list_widget.h"
#include "file/file_transfer_client.h"

class file_widget : public QWidget
{
    Q_OBJECT

   public:
    file_widget(std::string user, std::string password, std::string token, QWidget* parent = nullptr);
    ~file_widget() override;

   public:
    void startup(const QString& ip, uint16_t port);

   private:
    void on_new_file_clicked();

   Q_SIGNALS:
    void window_closed();
    void notify_event_signal(const leaf::notify_event& e);
    void upload_notify_signal(const leaf::upload_event& e);
    void download_notify_signal(const leaf::download_event& e);
    void cotrol_notify_signal(const leaf::cotrol_event& e);

   protected:
    void closeEvent(QCloseEvent* event) override;
   private Q_SLOTS:
    void on_notify_event(const leaf::notify_event& e);
    void on_error_occurred(const QString& error_msg);
    void on_upload_notify(const leaf::upload_event& e);
    void on_download_notify(const leaf::download_event& e);
    void on_cotrol_notify(const leaf::cotrol_event& e);
    void show_file_page();
    void show_upload_page();
    void on_upload_file();
    void on_new_folder();
    void on_breadcrumb_clicked();
    void on_rename(const QModelIndex& index, const QString& old_name, const QString& new_name);

   private:
    void login_notify(const leaf::notify_event& e);
    void logout_notify(const leaf::notify_event& e);
    void create_directory_notify(const leaf::notify_event& e);
    void rename_notify(const leaf::notify_event& e);
    void update_breadcrumb();
    void navigate_to_breadcrumb(QObject* obj);

    void notify_progress(const std::any& data);
    void upload_progress(const std::any& data);
    void cotrol_progress(const std::any& data);
    void download_progress(const std::any& data);
    void on_files(const leaf::files_response& files);
    void error_progress(const std::any& data);

   public:
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

   public:
    void setup_files_ui();
    void setup_upload_ui();
    void setup_login_ui();
    void setup_side_ui();
    void setup_connections();
    void view_double_clicked(const QModelIndex& index);
    void view_custom_context_menu_requested(const QPoint& pos);
    QToolButton* create_ellipsis_button(int start_index);
    QToolButton* create_breadcrumb_button(int index);
    void build_breadcrumb_path();
    void clear_breadcrumb_layout();

   private:
    std::string token_;
    std::string user_;
    std::string password_;
    QToolButton* files_btn_ = nullptr;
    QButtonGroup* btn_group_ = nullptr;

    QStackedWidget* stack_ = nullptr;
    QPoint last_click_pos_;
    //
    QWidget* file_page_ = nullptr;
    QWidget* breadcrumb_widget_ = nullptr;
    QHBoxLayout* breadcrumb_layout_ = nullptr;
    QPushButton* btn_file_page_;
    QPushButton* btn_upload_page_;
    QPushButton* new_folder_btn_;
    QPushButton* upload_file_btn_;
    QListView* view_ = nullptr;
    leaf::file_model* model_ = nullptr;

    QWidget* loading_overlay_;
    QLabel* loading_label_;
    QVBoxLayout* side_layout_ = nullptr;
    QVBoxLayout* main_layout = nullptr;
    QHBoxLayout* content_layout_ = nullptr;
    upload_list_widget* upload_list_widget_ = nullptr;
    std::shared_ptr<leaf::path_manager> path_manager_ = nullptr;
    std::vector<std::shared_ptr<leaf::directory>> breadcrumb_list_;
    std::shared_ptr<leaf::file_transfer_client> file_client_ = nullptr;
    std::map<std::string, std::shared_ptr<leaf::directory>> directory_cache_;
    std::unordered_map<std::string, std::shared_ptr<leaf::file_item>> item_map_;
};
#endif

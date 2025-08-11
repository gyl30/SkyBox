#ifndef LEAF_GUI_MAIN_WINDOW_H
#define LEAF_GUI_MAIN_WINDOW_H

#include <QWidget>
#include <QStackedWidget>
#include "loginwindow.h"
#include "gui/file_widget.h"
#include "gui/settingspage.h"

class main_window : public QWidget
{
    Q_OBJECT

   public:
    explicit main_window(QWidget *parent = nullptr);
    ~main_window() override;

   protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
   private slots:
    void on_minimize_clicked();
    void on_close_clicked();

   private:
    void handle_login_success(QString account, QString password, QString token);

   private:
    QPoint last_pos_;
    QPushButton *minimize_btn_ = nullptr;
    QPushButton *close_btn_ = nullptr;
    QPushButton *settings_btn_ = nullptr;

    QStackedWidget *stacked_widget_ = nullptr;
    login_window *login_ = nullptr;
    settings_window *settings_ = nullptr;
    file_widget *files_ = nullptr;
};

#endif

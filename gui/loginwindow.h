#ifndef LEAF_GUI_LOGIN_WINDOW_H
#define LEAF_GUI_LOGIN_WINDOW_H

#include <QWidget>
#include <boost/beast/core.hpp>
#include "net/executors.h"

class QLineEdit;
class QPushButton;

class login_window : public QWidget
{
    Q_OBJECT

   public:
    explicit login_window(QWidget *parent = nullptr);

   public:
    void on_settings(QString ip, uint16_t port);

   signals:
    void switch_to_other();
    void login_success(QString ip, uint16_t port, QString account, QString password, QString token);

   private slots:
    void on_settings_clicked();
    void on_login_clicked();
    void request_finished(const boost::beast::error_code &, const std::string &res);

   private:
    leaf::executors exs_{1};
    uint16_t port_ = 8080;
    QString address_{"127.0.0.1"};
    QLineEdit *account_ = nullptr;
    QLineEdit *password_ = nullptr;
    QPushButton *login_btn_ = nullptr;
};

#endif

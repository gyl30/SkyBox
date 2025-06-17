#ifndef LEAF_GUI_LOGINWIDGET_H
#define LEAF_GUI_LOGINWIDGET_H

#include <QWidget>
#include <QByteArray>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkAccessManager>

class QLineEdit;

class login_widget : public QWidget
{
    Q_OBJECT
   public:
    explicit login_widget(QWidget *parent = nullptr);

   private:
    void do_login();

   private slots:
    void login_clicked();
    void request_finished(QNetworkReply *);

   private:
    QLineEdit *user_ = nullptr;
    QLineEdit *pass_ = nullptr;
    QByteArray req_;
    QNetworkAccessManager *network_ = nullptr;
};

#endif

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QFontDatabase>
#include <QFormLayout>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <utility>
#include "log/log.h"
#include "protocol/codec.h"
#include "gui/loginwindow.h"
#include "protocol/message.h"

login_window::login_window(QWidget *parent) : QWidget(parent)
{
    account_ = new QLineEdit(this);
    account_->setFixedHeight(35);
    account_->setPlaceholderText("QQ号码/手机/邮箱");

    password_ = new QLineEdit(this);
    password_->setFixedHeight(35);
    password_->setEchoMode(QLineEdit::Password);
    password_->setPlaceholderText("密码");

    login_btn_ = new QPushButton("安全登录", this);
    login_btn_->setObjectName("loginButton");

    connect(login_btn_, &QPushButton::clicked, this, &login_window::on_login_clicked);

    network_ = new QNetworkAccessManager(this);
    connect(network_, &QNetworkAccessManager::finished, this, &login_window::request_finished);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addSpacing(20);
    layout->addWidget(account_);
    layout->addWidget(password_);

    layout->addSpacing(10);
    layout->addWidget(login_btn_);
    layout->addStretch();

    this->setLayout(layout);
}

void login_window::on_login_clicked()
{
    if (account_->text().isEmpty() || password_->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }
    login_btn_->setEnabled(false);
    login_btn_->setText("登录中...");
    LOG_INFO("login button clicked account {} password {}", account_->text().toStdString(), password_->text().toStdString());
    leaf::login_request login_request;
    login_request.username = account_->text().toStdString();
    login_request.password = password_->text().toStdString();
    auto data = leaf::serialize_login_request(login_request);
    QString bytes = QString::fromStdString(std::string(data.begin(), data.end()));
    req_ = bytes.toUtf8();
    QString url = "http://" + address_ + ":" + QString::number(port_) + "/leaf/login";
    LOG_INFO("request url {} content {}", url.toStdString(), std::string(data.begin(), data.end()));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    network_->post(request, req_);
}
void login_window::on_settings_clicked() { emit switch_to_other(); }

void login_window::on_settings(QString ip, uint16_t port)
{
    if (ip.isEmpty() || port == 0)
    {
        return;
    }
    address_ = std::move(ip);
    port_ = port;
}
void login_window::request_finished(QNetworkReply *reply)
{
    login_btn_->setEnabled(true);
    login_btn_->setText("安全登录");

    if (reply->error() == QNetworkReply::NoError)
    {
        QByteArray bytes = reply->readAll();
        auto l = leaf::deserialize_login_token(std::vector<uint8_t>(bytes.begin(), bytes.end()));
        if (!l.has_value())
        {
            QMessageBox::warning(this, "提示", "登录失败，数据错误");
            return;
        }
        std::string token = l->token;
        auto username = account_->text().toStdString();
        auto password = password_->text().toStdString();
        LOG_INFO("username {} password {} token {}", username, password, token);
        emit login_success(account_->text(), password_->text(), QString::fromStdString(token));
    }
    else
    {
        auto error_str = "登录失败" + reply->errorString();
        QMessageBox::warning(this, "提示", error_str);
    }
    reply->deleteLater();
}

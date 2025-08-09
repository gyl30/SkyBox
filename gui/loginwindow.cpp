#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "log/log.h"
#include "protocol/codec.h"
#include "gui/loginwindow.h"
#include "protocol/message.h"

login_widget::login_widget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout();
    auto *form = new QFormLayout();

    user_ = new QLineEdit(this);
    pass_ = new QLineEdit(this);
    pass_->setEchoMode(QLineEdit::Password);

    form->addRow("用户名:", user_);
    form->addRow("密码:", pass_);

    auto *login_btn = new QPushButton("登录", this);

    network_ = new QNetworkAccessManager(this);
    connect(network_, &QNetworkAccessManager::finished, this, &login_widget::request_finished);

    layout->addLayout(form);
    layout->addWidget(login_btn);
    setLayout(layout);
    connect(login_btn, &QPushButton::clicked, this, &login_widget::login_clicked);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    this->move((screenGeometry.width() - this->width()) / 2, (screenGeometry.height() - this->height()) / 2);
    resize(240, 180);
}

void login_widget::request_finished(QNetworkReply *reply)
{
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
        auto username = user_->text().toStdString();
        auto password = pass_->text().toStdString();
        LOG_INFO("username {} password {} token {}", username, password, token);
        if (widget == nullptr)
        {
            widget = new file_widget(username, password, token);
        }
        widget->startup();
        widget->show();
        this->hide();
    }
    else
    {
        auto error_str = "登录失败" + reply->errorString();
        QMessageBox::warning(this, "提示", error_str);
    }
    reply->deleteLater();
}

void login_widget::login_clicked()
{
    if (user_->text().isEmpty() || pass_->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }
    leaf::login_request login_request;
    login_request.username = user_->text().toStdString();
    login_request.password = pass_->text().toStdString();
    auto data = leaf::serialize_login_request(login_request);
    QString bytes = QString::fromStdString(std::string(data.begin(), data.end()));
    req_ = bytes.toUtf8();
    QString url = "http://127.0.0.1:8080/leaf/login";
    LOG_INFO("request url {} content {}", url.toStdString(), std::string(data.begin(), data.end()));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    network_->post(request, req_);
}

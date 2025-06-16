#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QScreen>
#include <QGuiApplication>

#include "loginwindow.h"

LoginWidget::LoginWidget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout();
    auto *form = new QFormLayout();

    user_ = new QLineEdit(this);
    pass_ = new QLineEdit(this);
    pass_->setEchoMode(QLineEdit::Password);

    form->addRow("用户名:", user_);
    form->addRow("密码:", pass_);

    auto *loginBtn = new QPushButton("登录", this);

    layout->addLayout(form);
    layout->addWidget(loginBtn);
    setLayout(layout);
    connect(loginBtn, &QPushButton::clicked, this, &LoginWidget::handleLoginClicked);
    const QRect screenGeometry = QGuiApplication::primaryScreen()->geometry();
    this->move((screenGeometry.width() - this->width()) / 2, (screenGeometry.height() - this->height()) / 2);
    resize(240, 180);
}

void LoginWidget::handleLoginClicked()
{
    if (user_->text().isEmpty() || pass_->text().isEmpty())
    {
        QMessageBox::warning(this, "提示", "用户名和密码不能为空");
        return;
    }
    this->hide();
}

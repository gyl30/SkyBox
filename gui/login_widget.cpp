#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialog>
#include <QMessageBox>

#include "gui/login_widget.h"

namespace leaf
{

login_dialog::login_dialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("登录");
    setFixedSize(500, 300);

    auto *username_label = new QLabel("用户名:");
    auto *username_edit = new QLineEdit();
    username_edit->setPlaceholderText("请输入用户名");

    // 密码输入框
    auto *password_label = new QLabel("密码:");
    auto *password_edit = new QLineEdit();
    password_edit->setEchoMode(QLineEdit::Password);
    password_edit->setPlaceholderText("请输入密码");

    auto *login_button = new QPushButton("登录");
    connect(login_button,
            &QPushButton::clicked,
            [=]()
            {
                QString username = username_edit->text();
                QString password = password_edit->text();

                if (username.isEmpty() || password.isEmpty())
                {
                    QMessageBox::warning(this, "输入错误", "用户名或密码不能为空！");
                    return;
                }
                emit login_data(username, password);

                accept();
            });

    auto *layout = new QVBoxLayout();
    layout->addWidget(username_label);
    layout->addWidget(username_edit);
    layout->addWidget(password_label);
    layout->addWidget(password_edit);
    layout->addWidget(login_button);

    setLayout(layout);
}

}    // namespace leaf

#include <QLineEdit>
#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include "settingspage.h"

settings_window::settings_window(QWidget* parent) : QWidget(parent)
{
    ip_ = new QLineEdit(this);
    ip_->setPlaceholderText("请输入IP地址");
    ip_->setFixedHeight(35);

    port_ = new QLineEdit(this);
    port_->setPlaceholderText("请输入端口号");
    port_->setFixedHeight(35);

    save_btn_ = new QPushButton("保存", this);
    save_btn_->setFixedHeight(35);
    connect(save_btn_, &QPushButton::clicked, this, &settings_window::on_save_clicked);

    auto* layout = new QVBoxLayout();
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addSpacing(20);
    layout->addWidget(ip_);
    layout->addWidget(port_);
    layout->addSpacing(10);
    layout->addWidget(save_btn_);
    layout->addStretch();
    setLayout(layout);
}

void settings_window::on_save_clicked()
{
    QString ip = ip_->text();
    QString port = port_->text();
    emit switch_to_other(ip, port.toInt());
}

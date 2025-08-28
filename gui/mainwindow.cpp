#include <QStackedWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QDebug>
#include <QFile>
#include <QFontDatabase>
#include <utility>
#include "gui/loginwindow.h"
#include "gui/mainwindow.h"

main_window::main_window(QWidget *parent) : QWidget(parent)
{
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    this->setAttribute(Qt::WA_TranslucentBackground, false);
    this->setStyleSheet("background-color: white;");
    this->setFixedSize(400, 300);

    login_ = new login_window();
    settings_ = new settings_window();
    auto *title_layout = new QHBoxLayout();

    settings_btn_ = new QPushButton("⚙", this);
    minimize_btn_ = new QPushButton("➖", this);
    close_btn_ = new QPushButton("❌", this);

    QFont emoji_font;
    emoji_font.setFamily("Segoe UI Emoji");
    if (QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFont(":/fonts/NotoColorEmoji.ttf")).isEmpty())
    {
        emoji_font.setFamily("Noto Color Emoji");
    }

    settings_btn_->setFont(emoji_font);
    minimize_btn_->setFont(emoji_font);
    close_btn_->setFont(emoji_font);

    QList<QPushButton *> buttons = {settings_btn_, minimize_btn_, close_btn_};
    for (QPushButton *btn : buttons)
    {
        btn->setFixedSize(35, 35);
        btn->setCursor(Qt::PointingHandCursor);
    }

    connect(minimize_btn_, &QPushButton::clicked, this, &main_window::on_minimize_clicked);
    connect(close_btn_, &QPushButton::clicked, this, &main_window::on_close_clicked);
    connect(settings_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentWidget(settings_); });
    connect(settings_,
            &settings_window::switch_to_other,
            this,
            [this](QString ip, uint16_t port)
            {
                login_->on_settings(std::move(ip), port);
                stacked_widget_->setCurrentWidget(login_);
            });

    title_layout->addStretch();
    title_layout->addWidget(settings_btn_);
    title_layout->addWidget(minimize_btn_);
    title_layout->addWidget(close_btn_);
    stacked_widget_ = new QStackedWidget(this);
    stacked_widget_->addWidget(login_);
    stacked_widget_->addWidget(settings_);

    stacked_widget_->setCurrentWidget(login_);

    auto *layout = new QVBoxLayout();
    layout->addLayout(title_layout);
    layout->addWidget(stacked_widget_);
    this->setLayout(layout);

    connect(login_,
            &login_window::switch_to_other,
            this,
            [this]()
            {
                qDebug() << "switchToSettings";

                stacked_widget_->setCurrentWidget(settings_);
            });
    connect(login_, &login_window::login_success, this, &main_window::handle_login_success);
}

main_window::~main_window() { delete files_; }

void main_window::handle_login_success(QString ip, uint16_t port, QString account, QString password, QString token)
{
    delete files_;

    files_ = new file_widget(account.toStdString(), password.toStdString(), token.toStdString());

    connect(files_, &file_widget::window_closed, this, &main_window::show);
    files_->startup(ip, port);
    files_->show();
    this->hide();
}

void main_window::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        last_pos_ = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void main_window::mouseMoveEvent(QMouseEvent *event)
{
    if ((event->buttons() & Qt::LeftButton) != 0U)
    {
        move(event->globalPosition().toPoint() - last_pos_);
        event->accept();
    }
}
void main_window::on_close_clicked() { this->close(); }

void main_window::on_minimize_clicked() { this->showMinimized(); }

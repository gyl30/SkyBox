#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMetaType>
#include <QTime>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStringList>
#include <QHeaderView>
#include <QLineEdit>
#include <QStyleFactory>
#include <QStyle>
#include <QPainter>
#include <QButtonGroup>
#include <QMap>
#include <QMessageBox>

#include "log/log.h"
#include "gui/task.h"
#include "gui/widget.h"
#include "gui/titlebar.h"
#include "gui/table_view.h"
#include "gui/table_model.h"
#include "gui/table_widget.h"
#include "gui/table_delegate.h"
#include "gui/files_widget.h"
#include "protocol/message.h"

static QIcon emoji_to_icon(const QString &emoji, int size = 32)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    QFont font("EmojiOne");
    font.setPointSizeF(size * 0.5);
    painter.setFont(font);
    painter.setPen(Qt::black);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, emoji);
    painter.end();
    return pixmap;
}
static void append_task_to_wiget(QTableWidget *table, const leaf::task &task, const QTime &t)
{
    // 0
    auto *filename_item = new QTableWidgetItem();
    filename_item->setText(QString::fromStdString(task.filename));
    filename_item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    // 1
    auto *op_item = new QTableWidgetItem();
    op_item->setText(QString::fromStdString(task.op));
    op_item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    // 2
    auto *size_item = new QTableWidgetItem();
    size_item->setText(QString::fromStdString(std::to_string(task.file_size)));
    size_item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    // 3
    auto *time_item = new QTableWidgetItem();
    time_item->setText(t.toString());
    time_item->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    //
    table->insertRow(table->rowCount());
    table->setItem(table->rowCount() - 1, 0, filename_item);
    table->setItem(table->rowCount() - 1, 1, op_item);
    table->setItem(table->rowCount() - 1, 2, size_item);
    table->setItem(table->rowCount() - 1, 3, time_item);
}

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    qRegisterMetaType<leaf::task>("leaf::task");
    qRegisterMetaType<leaf::gfile>("leaf::gfile");
    qRegisterMetaType<leaf::notify_event>("leaf::notify_event");
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    table_view_ = new leaf::task_table_view(this);

    model_ = new leaf::task_model();

    table_view_->setModel(model_);

    auto *delegate = new leaf::task_style_delegate();
    table_view_->setItemDelegateForColumn(1, delegate);

    // 用户登录区域
    user_label_ = new QLabel("用户名:");

    user_edit_ = new QLineEdit();
    user_edit_->setPlaceholderText("请输入用户名");
    user_edit_->setMinimumWidth(150);
    user_edit_->setFixedWidth(150);
    user_edit_->setMinimumHeight(30);

    key_label_ = new QLabel("密码:");
    key_edit_ = new QLineEdit();
    key_edit_->setEchoMode(QLineEdit::Password);
    key_edit_->setPlaceholderText("请输入密码");
    key_edit_->setMinimumWidth(150);
    key_edit_->setFixedWidth(150);
    key_edit_->setMinimumHeight(30);

    login_btn_ = new QToolButton();
    login_btn_->setText("登录");
    login_btn_->setMinimumWidth(80);
    login_btn_->setMinimumHeight(30);
    connect(login_btn_, &QToolButton::clicked, this, &Widget::on_login_btn_clicked);

    auto *user_layout = new QHBoxLayout();
    user_layout->setSpacing(10);
    user_layout->addStretch();
    user_layout->addWidget(user_label_);
    user_layout->addWidget(user_edit_);
    user_layout->addSpacing(20);
    user_layout->addWidget(key_label_);
    user_layout->addWidget(key_edit_);
    user_layout->addSpacing(20);
    user_layout->addWidget(login_btn_);
    user_layout->addStretch();

    finish_btn_ = new QToolButton(this);
    finish_btn_->setText("已完成");
    progress_btn_ = new QToolButton(this);
    progress_btn_->setText("上传中");
    upload_btn_ = new QToolButton(this);
    upload_btn_->setText("上传文件");
    files_btn_ = new QToolButton(this);
    files_btn_->setText("文件列表");
    style_btn_ = new QToolButton(this);
    style_btn_->setText("切换主题");

    files_btn_->setIcon(emoji_to_icon("📁", 64));
    upload_btn_->setIcon(emoji_to_icon("📤", 64));
    progress_btn_->setIcon(emoji_to_icon("⏳", 64));
    finish_btn_->setIcon(emoji_to_icon("✅", 64));
    style_btn_->setIcon(emoji_to_icon("🌈", 64));
    QToolButton *buttons[] = {finish_btn_, progress_btn_, upload_btn_, files_btn_, style_btn_};

    // 设置按钮样式
    for (QToolButton *btn : buttons)
    {
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        btn->setIconSize(QSize(64, 64));
    }

    btn_group_ = new QButtonGroup(this);
    btn_group_->setExclusive(true);
    btn_group_->addButton(finish_btn_);
    btn_group_->addButton(progress_btn_);
    btn_group_->addButton(upload_btn_);
    btn_group_->addButton(files_btn_);
    btn_group_->addButton(style_btn_);

    style_list_ = QStyleFactory::keys();
    finish_list_widget_ = new leaf::file_table_widget(this);
    stacked_widget_ = new QStackedWidget(this);
    files_widget_ = new leaf::files_widget(this);
    upload_list_index_ = stacked_widget_->addWidget(table_view_);
    finish_list_index_ = stacked_widget_->addWidget(finish_list_widget_);
    files_list_index_ = stacked_widget_->addWidget(files_widget_);

    finish_list_widget_->setSelectionBehavior(QAbstractItemView::SelectRows);     // 设置选中模式为选中行
    finish_list_widget_->setSelectionMode(QAbstractItemView::SingleSelection);    // 设置选中单个
    finish_list_widget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    finish_list_widget_->setShowGrid(false);
    finish_list_widget_->verticalHeader()->setHidden(true);
    finish_list_widget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(files_widget_, &leaf::files_widget::notify_event_signal, this, &Widget::on_notify_event_slot);
    // clang-format off
    connect(upload_btn_, &QPushButton::clicked, this, &Widget::on_new_file_clicked);
    connect(finish_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(finish_list_index_); });
    connect(progress_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(upload_list_index_); });
    connect(files_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(files_list_index_); });
    connect(style_btn_, &QPushButton::clicked, [this]() { on_style_btn_clicked(); });
    // clang-format on

    stacked_widget_->setCurrentIndex(files_list_index_);
    auto *side_layout = new QVBoxLayout();
    side_layout->addWidget(upload_btn_);
    side_layout->addWidget(style_btn_);
    side_layout->addWidget(files_btn_);
    side_layout->addWidget(progress_btn_);
    side_layout->addWidget(finish_btn_);

    auto *title_bar = new TitleBar(this);
    connect(title_bar, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
    connect(title_bar, &TitleBar::closeClicked, this, &QWidget::close);

    auto *content_layout = new QHBoxLayout();
    content_layout->addLayout(side_layout);
    content_layout->addWidget(stacked_widget_);
    content_layout->setSpacing(8);
    content_layout->setContentsMargins(0, 0, 0, 0);

    auto *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(title_bar);
    main_layout->addLayout(user_layout);
    main_layout->addLayout(content_layout);
    main_layout->setContentsMargins(8, 0, 0, 0);

    theme_names_ = QStyleFactory::keys();
    // 设置主题名称列表
    current_theme_index_ = 0;

    // 修改样式按钮的文本
    style_btn_->setText("切换主题");

    // 修改主题切换函数
    connect(style_btn_,
            &QPushButton::clicked,
            this,
            [this]()
            {
                current_theme_index_ = (current_theme_index_ + 1) % theme_names_.size();
                QString theme_name = theme_names_.at(current_theme_index_);
                QApplication::setStyle(QStyleFactory::create(theme_name));
            });

    progress_timer_ = new QTimer(this);
    progress_timer_->start(600);
    connect(progress_timer_, &QTimer::timeout, this, &Widget::update_progress_btn_icon);
    connect(this, &Widget::progress_slot, this, &Widget::on_progress_slot);
    connect(this, &Widget::notify_event_slot, this, &Widget::on_notify_event_slot);
    connect(this, &Widget::error_occurred, this, &Widget::on_error_occurred);

    resize(800, 600);
}
void Widget::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        click_pos_ = e->globalPos() - frameGeometry().topLeft();
        e->accept();
    }
}
void Widget::mouseMoveEvent(QMouseEvent *e)
{
    if ((e->buttons() & Qt::LeftButton) != 0U)
    {
        move(e->globalPos() - click_pos_);
        e->accept();
    }
}
void Widget::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
    {
        hide();
    }
    QWidget::mouseDoubleClickEvent(e);
}
void Widget::update_progress_btn_icon()
{
    QIcon icon = emoji_to_icon(hourglass_frames_[static_cast<int>(progress_frame_index_)], 64);
    progress_btn_->setIcon(icon);
    progress_frame_index_ = (progress_frame_index_ + 1) % hourglass_frames_.size();
}
void Widget::on_login_btn_clicked()
{
    auto user = user_edit_->text();
    auto key = key_edit_->text();
    if (user.isEmpty() || key.isEmpty())
    {
        QMessageBox::warning(this, "警告", "用户名和密码不能为空");
        return;
    }
    if (file_client_ != nullptr)
    {
        return;
    }
    login_btn_->setText("登录中...");
    login_btn_->setEnabled(false);
    user_edit_->setEnabled(false);
    key_edit_->setEnabled(false);

    leaf::progress_handler handler;
    handler.u.upload = [this](const leaf::upload_event &e) { upload_progress(e); };
    handler.d.download = [this](const leaf::download_event &e) { download_progress(e); };
    handler.u.notify = [this](const leaf::notify_event &e) { notify_progress(e); };
    handler.d.notify = [this](const leaf::notify_event &e) { notify_progress(e); };
    handler.c.notify = [this](const leaf::notify_event &e) { notify_progress(e); };
    handler.u.error = [this](const boost::system::error_code &ec) { error_progress(ec); };
    handler.d.error = [this](const boost::system::error_code &ec) { error_progress(ec); };
    handler.c.error = [this](const boost::system::error_code &ec) { error_progress(ec); };

    file_client_ = std::make_shared<leaf::file_transfer_client>("127.0.0.1", 8080, user.toStdString(), key.toStdString(), handler);
    file_client_->startup();
}

void Widget::on_style_btn_clicked()
{
    if (style_index_ >= style_list_.size())
    {
        style_index_ = 0;
    }
    qApp->setStyle(QStyleFactory::create(style_list_.at(style_index_)));
    style_index_++;
}
void Widget::on_progress_slot(const leaf::task &e)
{
    LOG_INFO("{} progress {} {} {}", e.op, e.filename, e.process_size, e.file_size);
    if (e.process_size == e.file_size && e.file_size != 0)
    {
        model_->delete_task(e);
        append_task_to_wiget(finish_list_widget_, e, QTime::currentTime());
        return;
    }

    model_->add_or_update_task(e);
}

void Widget::download_progress(const leaf::download_event &e)
{
    leaf::task t;
    t.file_size = e.file_size;
    t.filename = e.filename;
    t.process_size = e.download_size;
    t.op = "download";
    emit progress_slot(t);
}

void Widget::notify_progress(const leaf::notify_event &e) { emit notify_event_slot(e); }

void Widget::error_progress(const boost::system::error_code &ec)
{
    QString error_msg = QString::fromStdString(ec.message());
    emit error_occurred(error_msg);
}

void Widget::on_error_occurred(const QString &error_msg)
{
    LOG_ERROR("error {}", error_msg.toStdString());

    // 显示错误消息给用户
    QMessageBox::critical(this, "错误", error_msg);

    // 执行清理操作
    if (file_client_)
    {
        file_client_->shutdown();
        file_client_.reset();
    }

    // 重置 UI 状态
    reset_ui_state();
}

void Widget::reset_ui_state()
{
    login_btn_->setText("登录");
    login_btn_->setEnabled(true);
    user_edit_->setEnabled(true);
    key_edit_->setEnabled(true);
    user_edit_->clear();
    key_edit_->clear();
}

static void files_to_gfiles(const std::vector<leaf::file_node> &files, int dep, std::vector<leaf::gfile> &gfiles)
{
    if (dep > 3)
    {
        return;
    }
    for (const auto &f : files)
    {
        leaf::gfile gf;
        gf.filename = f.name;
        gf.parent = f.parent;
        gf.type = f.type;
        gfiles.push_back(gf);
        LOG_DEBUG("on_files_response file {} type {} parent {}", f.name, f.type, f.parent);
    }
}

void Widget::on_files(const std::vector<leaf::file_node> &files)
{
    std::vector<leaf::gfile> gfiles;
    files_to_gfiles(files, 0, gfiles);
    for (const auto &f : gfiles)
    {
        files_widget_->add_or_update_file(f);
    }
}

void Widget::on_notify_event_slot(const leaf::notify_event &e)
{
    if (e.method == "files")
    {
        on_files(std::any_cast<std::vector<leaf::file_node>>(e.data));
    }
    if (e.method == "rename")
    {
        rename_notify(e);
    }
    if (e.method == "new_directory")
    {
        new_directory_notify(e);
    }
    if (e.method == "change_directory")
    {
        change_directory_notify(e);
    }
    if (e.method == "login")
    {
        login_notify(e);
    }
    if (e.method == "logout")
    {
        logout_notify(e);
    }
}

void Widget::change_directory_notify(const leaf::notify_event &e)
{
    if (file_client_)
    {
        auto dir = std::any_cast<std::string>(e.data);
        file_client_->change_current_dir(dir);
    }
}

void Widget::new_directory_notify(const leaf::notify_event &e)
{
    if (file_client_)
    {
        auto dir = std::any_cast<std::string>(e.data);
        file_client_->create_directory(dir);
    }
}

void Widget::rename_notify(const leaf::notify_event &e) {}

void Widget::login_notify(const leaf::notify_event &e)
{
    bool login = std::any_cast<bool>(e.data);
    if (login)
    {
        login_btn_->setText("已登录");
        user_edit_->setEnabled(false);
        key_edit_->setEnabled(false);
        login_btn_->setEnabled(false);
    }
    else
    {
        login_btn_->setText("登录");
        user_edit_->setEnabled(true);
        key_edit_->setEnabled(true);

        login_btn_->setEnabled(true);
    }
}
void Widget::logout_notify(const leaf::notify_event &e)
{
    login_btn_->setText("登录");
    user_edit_->setEnabled(true);
    key_edit_->setEnabled(true);
    login_btn_->setEnabled(true);
}

void Widget::upload_progress(const leaf::upload_event &e)
{
    leaf::task t;
    t.file_size = e.file_size;
    t.filename = e.filename;
    t.process_size = e.upload_size;
    t.op = "upload";
    emit progress_slot(t);
}

Widget::~Widget()
{
    if (file_client_ != nullptr)
    {
        file_client_->shutdown();
        file_client_.reset();
    }
}

void Widget::on_new_file_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, "选择文件");
    if (filename.isEmpty())
    {
        return;
    }
    file_client_->add_upload_file(filename.toStdString());
}

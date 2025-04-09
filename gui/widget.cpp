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

#include "log/log.h"
#include "gui/task.h"
#include "gui/widget.h"
#include "gui/titlebar.h"
#include "gui/table_view.h"
#include "gui/table_model.h"
#include "gui/table_widget.h"
#include "gui/login_widget.h"
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

    finish_btn_ = new QToolButton(this);
    finish_btn_->setText("已完成");
    progress_btn_ = new QToolButton(this);
    progress_btn_->setText("上传中");
    upload_btn_ = new QToolButton(this);
    upload_btn_->setText("上传文件");
    login_btn_ = new QToolButton(this);
    login_btn_->setText("登录");
    files_btn_ = new QToolButton(this);
    files_btn_->setText("文件列表");
    style_btn_ = new QToolButton(this);
    style_btn_->setText("切换主题");

    files_btn_->setIcon(emoji_to_icon("📁", 64));
    login_btn_->setIcon(emoji_to_icon("✨", 64));
    upload_btn_->setIcon(emoji_to_icon("📤", 64));
    progress_btn_->setIcon(emoji_to_icon("⏳", 64));
    finish_btn_->setIcon(emoji_to_icon("✅", 64));
    style_btn_->setIcon(emoji_to_icon("🌈", 64));
    QToolButton *buttons[] = {finish_btn_, progress_btn_, upload_btn_, login_btn_, files_btn_, style_btn_};

    // 设置按钮样式
    for (QToolButton *btn : buttons)
    {
        btn->setCheckable(true);
        btn->setAutoRaise(true);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        btn->setIconSize(QSize(64, 64));
        btn->setFixedSize(64, 64);
        btn->setStyleSheet(R"(
        QToolButton {
            background-color: transparent;
            border: none;
            border-radius: 8px;
        }
        QToolButton:hover {
            background-color: rgba(0, 0, 0, 20);
        }
        QToolButton:checked {
            background-color: rgba(0, 120, 215, 50);
        }
    )");
    }

    btn_group_ = new QButtonGroup(this);
    btn_group_->setExclusive(true);
    btn_group_->addButton(finish_btn_);
    btn_group_->addButton(progress_btn_);
    btn_group_->addButton(upload_btn_);
    btn_group_->addButton(login_btn_);
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
    // clang-format off
    connect(upload_btn_, &QPushButton::clicked, this, &Widget::on_new_file_clicked);
    connect(finish_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(finish_list_index_); });
    connect(progress_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(upload_list_index_); });
    connect(login_btn_, &QPushButton::clicked, this, &Widget::setting_btn_clicked);
    connect(files_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(files_list_index_); });
    connect(style_btn_, &QPushButton::clicked, [this]() { on_style_btn_clicked(); });
    // clang-format on
    stacked_widget_->setCurrentIndex(files_list_index_);
    auto *side_layout = new QVBoxLayout();
    side_layout->addWidget(upload_btn_);
    side_layout->addWidget(login_btn_);
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
    main_layout->addLayout(content_layout);
    main_layout->setContentsMargins(8, 0, 0, 0);
    // 设置窗口样式
    setStyleSheet(R"(
        QWidget {
            background: white;
            font-family: "Microsoft YaHei";
        }
        QWidget#MainWindow {
            border: 1px solid #E0E0E0;
            border-radius: 4px;
        }
        QToolButton#SidebarNavButton {
            background: #F5F5F5;
            border: none;
            padding: 8px;
            color: #666;
            font-size: 12px;
            border-radius: 4px;
            margin: 2px;
            min-width: 80px;
            max-width: 80px;
        }
        QToolButton#SidebarNavButton:hover {
            background: #E0E0E0;
        }
        QToolButton#SidebarNavButton:checked {
            background: #2196F3;
            color: white;
        }
        QTableWidget {
            background: white;
            border: 1px solid #E0E0E0;
            border-radius: 4px;
            gridline-color: #E0E0E0;
        }
        QTableWidget::item {
            padding: 8px;
        }
        QHeaderView::section {
            background: #F5F5F5;
            padding: 8px;
            border: none;
            border-bottom: 1px solid #E0E0E0;
        }
        QScrollBar:vertical {
            border: none;
            background: #F5F5F5;
            width: 8px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #BDBDBD;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #9E9E9E;
        }
    )");
    setLayout(main_layout);
    resize(800, 500);
    progress_timer_ = new QTimer(this);
    progress_timer_->start(600);
    connect(progress_timer_, &QTimer::timeout, this, &Widget::update_progress_btn_icon);
    connect(this, &Widget::progress_slot, this, &Widget::on_progress_slot);
    connect(this, &Widget::notify_event_slot, this, &Widget::on_notify_event_slot);
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
void Widget::setting_btn_clicked()
{
    LOG_INFO("setting btn clicked");
    leaf::login_dialog login_dialog(this);
    connect(&login_dialog, &leaf::login_dialog::login_data, this, &Widget::on_login_slot);
    login_dialog.exec();
}
void Widget::on_login_slot(const QString &user, const QString &passwd)
{
    if (user.isEmpty() || passwd.isEmpty())
    {
        return;
    }
    if (file_client_ != nullptr)
    {
        return;
    }
    leaf::progress_handler handler;
    handler.upload = [this](const leaf::upload_event &e) { upload_progress(e); };
    handler.download = [this](const leaf::download_event &e) { download_progress(e); };
    handler.notify = [this](const leaf::notify_event &e) { notify_progress(e); };

    file_client_ = new leaf::file_transfer_client("127.0.0.1", 8080, handler);
    file_client_->startup();

    file_client_->login(user.toStdString(), passwd.toStdString());
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
    LOG_INFO("{} progress {} {} {} {}", e.op, e.id, e.filename, e.process_size, e.file_size);
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
    t.id = e.id;
    t.filename = e.filename;
    t.process_size = e.download_size;
    t.op = "download";
    emit progress_slot(t);
}

void Widget::notify_progress(const leaf::notify_event &e) { emit notify_event_slot(e); }

static void files_to_gfiles(const std::vector<leaf::files_response::file_node> &files,
                            int dep,
                            std::vector<leaf::gfile> &gfiles)
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

void Widget::on_files(const leaf::files_response &files)
{
    std::vector<leaf::gfile> gfiles;
    files_to_gfiles(files.files, 0, gfiles);
    for (const auto &f : gfiles)
    {
        files_widget_->add_or_update_file(f);
    }
}

void Widget::on_notify_event_slot(const leaf::notify_event &e)
{
    if (e.method == "files")
    {
        on_files(std::any_cast<leaf::files_response>(e.data));
    }
    if (e.method == "login")
    {
        login_btn_->hide();
    }
    if (e.method == "logout")
    {
        login_btn_->show();
    }
}

void Widget::upload_progress(const leaf::upload_event &e)
{
    leaf::task t;
    t.file_size = e.file_size;
    t.id = e.id;
    t.filename = e.filename;
    t.process_size = e.upload_size;
    t.op = "upload";
    emit progress_slot(t);
}

Widget::~Widget()
{
    file_client_->shutdown();
    delete file_client_;
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

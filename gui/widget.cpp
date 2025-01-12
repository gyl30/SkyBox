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

#include "log/log.h"
#include "gui/task.h"
#include "gui/widget.h"
#include "gui/table_view.h"
#include "gui/table_model.h"
#include "gui/table_widget.h"
#include "gui/login_widget.h"
#include "gui/table_delegate.h"
#include "gui/files_widget.h"
#include "protocol/message.h"

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
    qRegisterMetaType<leaf::notify_event>("leaf::notify_event");

    table_view_ = new leaf::task_table_view(this);

    model_ = new leaf::task_model();

    table_view_->setModel(model_);

    auto *delegate = new leaf::task_style_delegate();
    table_view_->setItemDelegateForColumn(1, delegate);

    finish_btn_ = new QPushButton(this);
    finish_btn_->setText("已完成");
    progress_btn_ = new QPushButton(this);
    progress_btn_->setText("上传中");
    upload_btn_ = new QPushButton(this);
    upload_btn_->setText("上传文件");
    setting_btn_ = new QPushButton(this);
    setting_btn_->setText("设置");
    files_btn_ = new QPushButton(this);
    files_btn_->setText("文件列表");

    style_list_ = QStyleFactory::keys();
    style_btn_ = new QPushButton(this);
    style_btn_->setText("切换主题");
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
    connect(setting_btn_, &QPushButton::clicked, this, &Widget::setting_btn_clicked);
    connect(files_btn_, &QPushButton::clicked, this, [this]() { stacked_widget_->setCurrentIndex(files_list_index_); });
    connect(style_btn_, &QPushButton::clicked, [this]() { on_style_btn_clicked(); });
    // clang-format on

    auto *main_layout = new QVBoxLayout();
    auto *top_layout = new QHBoxLayout();
    top_layout->addWidget(files_btn_);
    top_layout->addWidget(progress_btn_);
    top_layout->addWidget(finish_btn_);
    top_layout->addWidget(upload_btn_);
    top_layout->addWidget(setting_btn_);
    top_layout->addWidget(style_btn_);
    top_layout->setSpacing(0);
    main_layout->addLayout(top_layout);
    main_layout->addWidget(stacked_widget_);

    setLayout(main_layout);
    resize(800, 500);
    auto upload_cb = [this](const leaf::upload_event &e) { upload_progress(e); };
    auto download_cb = [this](const leaf::download_event &e) { download_progress(e); };
    auto notify_cb = [this](const leaf::notify_event &e) { notify_progress(e); };
    file_client_ = new leaf::file_transfer_client("127.0.0.1", 8080, upload_cb, download_cb, notify_cb);
    file_client_->startup();
    connect(this, &Widget::progress_slot, this, &Widget::on_progress_slot);
    connect(this, &Widget::notify_event_slot, this, &Widget::on_notify_event_slot);
}

void Widget::setting_btn_clicked()
{
    LOG_INFO("setting btn clicked");
    leaf::login_dialog login_dialog(this);
    connect(&login_dialog, &leaf::login_dialog::login_data, this, &Widget::on_login_slot);
    login_dialog.exec();
}
void Widget::on_login_slot(QString user, QString passwd)
{
    LOG_INFO("login {} {}", user.toStdString(), passwd.toStdString());
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
void Widget::on_progress_slot(leaf::task e)
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
        LOG_DEBUG("on_files_response file {} type {} children size {}", f.name, f.type, f.children.size());
        if (!f.children.empty())
        {
            files_to_gfiles(f.children, dep++, gfiles);
        }
    }
}

void Widget::on_files(const leaf::files_response &files)
{
    std::vector<leaf::gfile> gfiles;
    files_to_gfiles(files.files, 0, gfiles);
    files_widget_->add_gfiles(gfiles);
    files_widget_->update();
}

void Widget::on_notify_event_slot(leaf::notify_event e) { on_files(std::any_cast<leaf::files_response>(e.data)); }

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

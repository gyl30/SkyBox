#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMetaType>

#include "qt/task.h"
#include "qt/widget.h"
#include "qt/table_view.h"
#include "qt/table_model.h"
#include "qt/table_delegate.h"

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    qRegisterMetaType<leaf::task>("leaf::task");
    table_view_ = new leaf::task_table_view(this);

    model_ = new leaf::task_model();

    table_view_->setModel(model_);

    auto *delegate = new leaf::task_style_delegate();
    table_view_->setItemDelegateForColumn(1, delegate);

    auto *new_file_btn = new QPushButton();
    new_file_btn->setText("Add New File");
    auto conn = connect(new_file_btn, &QPushButton::clicked, this, &Widget::on_new_file_clicked);
    (void)conn;
    auto *layout = new QHBoxLayout();
    layout->addWidget(new_file_btn);
    layout->addWidget(table_view_);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
    resize(800, 300);
    auto upload_cb = [this](const leaf::upload_event &e) { upload_progress(e); };
    auto download_cb = [this](const leaf::download_event &e) { download_progress(e); };
    file_client_ = new leaf::file_transfer_client("127.0.0.1", 8080, upload_cb, download_cb);
    file_client_->startup();
    connect(this, &Widget::progress_slot, this, &Widget::on_progress_slot);
}

void Widget::on_progress_slot(leaf::task e) { model_->add_or_update_task(e); }

void Widget::download_progress(const leaf::download_event &e)
{
    LOG_INFO("--> download progress {} {} {} {}", e.id, e.filename, e.download_size, e.file_size);
    leaf::task t;
    t.file_size = e.file_size;
    t.id = e.id;
    t.filename = e.filename;
    t.process_size = e.download_size;
    t.op = "upload";
    emit progress_slot(t);
}

void Widget::upload_progress(const leaf::upload_event &e)
{
    LOG_INFO("<-- upload progress {} {} {} {}", e.id, e.filename, e.upload_size, e.file_size);
    leaf::task t;
    t.file_size = e.file_size;
    t.id = e.id;
    t.filename = e.filename;
    t.process_size = e.upload_size;
    t.op = "upload";
    emit progress_slot(t);
}

Widget::~Widget() { file_client_->shutdown(); }

void Widget::on_new_file_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, "选择文件");
    if (filename.isEmpty())
    {
        return;
    }
    file_client_->add_upload_file(filename.toStdString());
}

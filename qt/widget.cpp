#include <QApplication>
#include <QPushButton>
#include <QHBoxLayout>
#include <QFileDialog>

#include "widget.h"
#include "task_item.h"
#include "table_view.h"
#include "table_model.h"
#include "table_delegate.h"

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    table_view_ = new leaf::task_table_view(this);

    model_ = new leaf::task_model();

    table_view_->setModel(model_);

    auto *delegate = new leaf::task_style_delegate();
    table_view_->setItemDelegateForColumn(4, delegate);

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
}

Widget::~Widget() = default;

void Widget::on_new_file_clicked()
{
    auto filename = QFileDialog::getOpenFileName(this, "选择文件");
    if (filename.isEmpty())
    {
        return;
    }
    leaf::file_item t;
    t.name = filename.toStdString();
    t.dst_file = t.name + ".leaf";
    auto file = std::make_shared<leaf::file_task>(t);
    model_->add_task(file);
}

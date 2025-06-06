#include <QVBoxLayout>
#include <QListWidgetItem>
#include "upload_list_widget.h"

upload_list_widget::upload_list_widget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    list_widget_ = new QListWidget(this);
    list_widget_->setAlternatingRowColors(true);

    layout->addWidget(list_widget_);
    setLayout(layout);
}

void upload_list_widget::add_task_to_view(const leaf::upload_event &e)
{
    for (int i = 0; i < list_widget_->count(); ++i)
    {
        QListWidgetItem *item = list_widget_->item(i);
        auto event_item = item->data(Qt::UserRole).value<leaf::upload_event>();
        if (event_item.filename != e.filename || event_item.file_size != e.file_size)
        {
            return;
        }
        item->setData(Qt::UserRole, QVariant::fromValue<leaf::upload_event>(e));
    }

    auto *list_item = new QListWidgetItem();
    list_item->setData(Qt::UserRole, QVariant::fromValue<leaf::upload_event>(e));
    auto index = list_widget_->count();
    list_widget_->insertItem(index, list_item);
}

void upload_list_widget::remove_task_from_view(const leaf::upload_event &e)
{
    for (int i = 0; i < list_widget_->count(); ++i)
    {
        QListWidgetItem *item = list_widget_->item(i);
        auto event_item = item->data(Qt::UserRole).value<leaf::upload_event>();
        if (event_item.filename != e.filename || event_item.file_size != e.file_size)
        {
            continue;
        }
        list_widget_->takeItem(i);
        delete item;
        return;
    }
}

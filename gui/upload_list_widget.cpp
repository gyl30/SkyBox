#include <QListView>
#include <QVBoxLayout>
#include "log/log.h"
#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_task_model.h"
#include "gui/upload_list_widget.h"
#include "gui/upload_item_delegate.h"

upload_list_widget::upload_list_widget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_view_ = new QListView(this);
    model_ = new upload_task_model(this);
    delegate_ = new upload_item_delegate(this);

    list_view_->setModel(model_);
    list_view_->setItemDelegate(delegate_);

    list_view_->setAlternatingRowColors(true);
    list_view_->setSpacing(1);
    list_view_->setUniformItemSizes(true);
    list_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    list_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    list_view_->setFocusPolicy(Qt::ClickFocus);

    layout->addWidget(list_view_);
    setLayout(layout);

    connect(delegate_, &upload_item_delegate::pause_button_clicked, this, &upload_list_widget::on_pause_button_clicked);
    connect(delegate_, &upload_item_delegate::cancel_button_clicked, this, &upload_list_widget::on_cancel_button_clicked);
}

void upload_list_widget::add_task_to_view(const leaf::upload_event &e) { model_->add_task(e); }

void upload_list_widget::remove_task_from_view(const leaf::upload_event &e) { model_->remove_task(e); }

void upload_list_widget::on_pause_button_clicked(const QModelIndex &index)
{
    (void)this;
    if (!index.isValid())
    {
        return;
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();

    LOG_INFO("action button clicked for file: {}", task.filename);
}

void upload_list_widget::on_cancel_button_clicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::upload_event>();

    model_->remove_task(task);

    LOG_INFO("cancel button clicked for file: {}", task.filename);
}

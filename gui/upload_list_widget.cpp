#include <QListView>
#include <QVBoxLayout>
#include "log/log.h"
#include "gui/util.h"
#include "file/event.h"
#include "gui/upload_task_model.h"
#include "gui/upload_list_widget.h"
#include "gui/upload_item_delegate.h"

file_list_widget::file_list_widget(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_view_ = new QListView(this);
    model_ = new file_task_model(this);
    delegate_ = new file_item_delegate(this);

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

    connect(delegate_, &file_item_delegate::pause_button_clicked, this, &file_list_widget::on_pause_button_clicked);
    connect(delegate_, &file_item_delegate::cancel_button_clicked, this, &file_list_widget::on_cancel_button_clicked);

    connect(model_,
            &QAbstractItemModel::rowsInserted,
            this,
            [this](const QModelIndex &parent, int first, int last)
            {
                for (int i = first; i <= last; ++i)
                {
                    QModelIndex index = model_->index(i, 0, parent);
                    if (index.isValid())
                    {
                        list_view_->openPersistentEditor(index);
                    }
                }
            });
}

void file_list_widget::add_task_to_view(const leaf::file_event &e) { model_->add_task(e); }

void file_list_widget::remove_task_from_view(const leaf::file_event &e) { model_->remove_task(e); }

void file_list_widget::on_pause_button_clicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::file_event>();

    LOG_INFO("pause button clicked for file: {}", task.filename);
}

void file_list_widget::on_cancel_button_clicked(const QModelIndex &index)
{
    if (!index.isValid())
    {
        return;
    }

    auto task = index.data(static_cast<int>(leaf::task_role::kFullEventRole)).value<leaf::file_event>();

    LOG_INFO("cancel button clicked for file: {}", task.filename);
}

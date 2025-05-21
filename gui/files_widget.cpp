#include <QMenu>
#include <QLabel>
#include <QIcon>
#include <QString>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QApplication>
#include <filesystem>

#include "log/log.h"
#include "gui/files_widget.h"

namespace leaf
{
files_widget::files_widget(QWidget *parent) : QWidget(parent)
{
    list_widget_ = new QListWidget(this);
    list_widget_->setWordWrap(true);
    list_widget_->setDragDropMode(QAbstractItemView::InternalMove);
    list_widget_->setIconSize(QSize(100, 100));
    list_widget_->setGridSize(QSize(200, 200));
    list_widget_->setUniformItemSizes(true);
    list_widget_->setViewMode(QListView::IconMode);
    list_widget_->setFlow(QListView::LeftToRight);
    list_widget_->setMovement(QListView::Snap);
    list_widget_->setLayoutMode(QListView::Batched);
    list_widget_->setSpacing(0);
    list_widget_->setEditTriggers(QAbstractItemView::DoubleClicked);

    connect(list_widget_, &QListWidget::itemChanged, this, &files_widget::on_item_changed);
    connect(list_widget_, &QListWidget::itemDoubleClicked, this, &files_widget::on_item_double_clicked);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(list_widget_);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

files_widget::~files_widget() {}

void files_widget::add_or_update_file(const leaf::gfile &file)
{
    if (file.parent == file.filename && file.type == "dir")
    {
        return;
    }
    if (current_path_.empty())
    {
        current_path_ = file.parent;
    }
    const auto &files = gfiles_[file.parent];
    for (const auto &f : files)
    {
        if (f.filename == file.filename && f.type == file.type && f.parent == file.parent)
        {
            return;
        }
    }
    gfiles_[file.parent].push_back(file);
    if (file.parent != current_path_)
    {
        return;
    }
    QIcon icon;
    if (file.type == "dir")
    {
        icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    }
    else
    {
        icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    }

    std::filesystem::path p(file.filename);
    auto *item = new QListWidgetItem(icon, QString::fromStdString(p.filename().string()));
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    list_widget_->addItem(item);
}

void files_widget::contextMenuEvent(QContextMenuEvent *event)
{
    QListWidgetItem *item = list_widget_->itemAt(event->pos());
    QMenu menu;
    QAction *rename_action = nullptr;
    if (item != nullptr)
    {
        rename_action = menu.addAction("重命名");
    }
    QAction *new_dir_action = menu.addAction("新建文件夹");
    QAction *selected_action = menu.exec(event->globalPos());
    if (rename_action != nullptr && selected_action == rename_action)
    {
        old_filename_ = item->text().toStdString();
        list_widget_->editItem(item);
    }
    else if (selected_action == new_dir_action)
    {
        auto icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        auto *item = new QListWidgetItem(icon, QString("新建文件夹"));
        item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        list_widget_->addItem(item);
        LOG_INFO("new directory clicked");
        leaf::notify_event e;
        e.method = "new_directory";
        e.data = item->text().toStdString();
        emit notify_event_signal(e);
    }
}

void files_widget::on_item_double_clicked(QListWidgetItem *item)
{
    std::string filename = item->text().toStdString();
    LOG_INFO("item double clicked {}", filename);
    leaf::notify_event e;
    e.method = "change_directory";
    e.data = filename;
    emit notify_event_signal(e);
}
void files_widget::on_item_changed(QListWidgetItem *item)
{
    std::string filename = item->text().toStdString();
    LOG_INFO("item changed from {} to {}", old_filename_, filename);
    leaf::notify_event e;
    e.method = "rename";
    e.data = filename;
    emit notify_event_signal(e);
}
}    // namespace leaf

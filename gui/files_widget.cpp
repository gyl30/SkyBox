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

    new_file_action_ = new QAction("新建文件", this);
    new_directory_action_ = new QAction("新建文件夹", this);
    connect(new_file_action_, &QAction::triggered, this, &files_widget::on_new_file_clicked);
    connect(new_directory_action_, &QAction::triggered, this, &files_widget::on_new_directory_clicked);

    auto *layout = new QHBoxLayout(this);
    layout->addWidget(list_widget_);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);
}

files_widget::~files_widget()
{
    delete new_file_action_;
    delete new_directory_action_;
}

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
    // item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    list_widget_->addItem(item);
}

void files_widget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(new_file_action_);
    menu.addAction(new_directory_action_);
    menu.exec(event->globalPos());
}

void files_widget::on_new_file_clicked()
{
    auto icon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    auto *item = new QListWidgetItem(icon, QString("新建文件"));
    // item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    list_widget_->addItem(item);
    LOG_INFO("new file clicked");
    leaf::notify_event e;
    e.method = "new_file";
    emit notify_event_signal(e);
}

void files_widget::on_new_directory_clicked()
{
    auto icon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);

    auto *item = new QListWidgetItem(icon, QString("新建文件夹"));
    // item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    list_widget_->addItem(item);
    LOG_INFO("new directory clicked");
    leaf::notify_event e;
    e.method = "new_directory";
    emit notify_event_signal(e);
}

}    // namespace leaf

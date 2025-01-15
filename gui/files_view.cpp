#include <QMenu>
#include <QLabel>
#include <QIcon>
#include <QString>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileIconProvider>

#include <filesystem>

#include "log/log.h"
#include "gui/files_view.h"
#include <qheaderview.h>

namespace leaf
{
files_view::files_view(QWidget *parent) : QTableView(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setShowGrid(false);
    verticalHeader()->setVisible(false);
    horizontalHeader()->setVisible(false);
    horizontalHeader()->setDefaultAlignment(Qt::AlignHCenter);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);

    rename_action_ = new QAction("重命名", this);
    new_directory_action_ = new QAction("新建文件夹", this);
    connect(rename_action_, &QAction::triggered, this, &files_view::on_new_file_clicked);
    connect(new_directory_action_, &QAction::triggered, this, &files_view::on_new_directory_clicked);
}

files_view::~files_view()
{
    delete rename_action_;
    delete new_directory_action_;
}

void files_view::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(rename_action_);
    menu.addAction(new_directory_action_);
    menu.exec(event->globalPos());
}

void files_view::on_new_file_clicked() { LOG_INFO("new file clicked"); }

void files_view::on_new_directory_clicked() { LOG_INFO("new directory clicked"); }

}    // namespace leaf

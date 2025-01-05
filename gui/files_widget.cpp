#include <QMenu>
#include "log/log.h"
#include "gui/files_widget.h"

namespace leaf
{
files_widget::files_widget(QWidget *parent) : QWidget(parent)
{
    rename_action_ = new QAction("重命名", this);
    new_directory_action_ = new QAction("新建文件夹", this);
    connect(rename_action_, &QAction::triggered, this, &files_widget::on_new_file_clicked);
    connect(new_directory_action_, &QAction::triggered, this, &files_widget::on_new_directory_clicked);
}
files_widget::~files_widget()
{
    delete rename_action_;
    delete new_directory_action_;
}

void files_widget::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);
    menu.addAction(rename_action_);
    menu.addAction(new_directory_action_);
    menu.exec(event->globalPos());
}

void files_widget::on_new_file_clicked() { LOG_INFO("new file clicked"); }

void files_widget::on_new_directory_clicked() { LOG_INFO("new directory clicked"); }

}    // namespace leaf
